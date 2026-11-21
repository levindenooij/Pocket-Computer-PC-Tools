#  hp15c_img_to_c15.py
#  This program converts an image of the HP-15C display to a C15 source file.
#  The image file is a PNG or JPG file that contains the keystrokes of a HP15C program.
#  The C15 source file is a formatted file that can be loaded into the VoyagerCalc HP-15C simulator.
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  General Usage 
#  hp15c_img_to_c15.py arg=filename.
#  Required directory structure in woking directory (for convenience)
#  py: where the python programs are stored
#  txt: for hp15c keystroke files
#  15c: for hp15c formatted source files
#  bin15c: for hp15c memory files
#  res: translation files for token translation	
#
#  Credits to :
#  Philip Houbert https://clones.phweb.me/pages/15interf.php
#  who provided input for the translation of the dump codes to C15 code and function names in his spreadsheet. 
#     

import sys
import csv
import json
import platform
import re
import subprocess
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent
IMAGE_DIR = PROJECT_ROOT / "img"
OUTPUT_DIR = PROJECT_ROOT / "15c"
INSTRUCTION_MAP = PROJECT_ROOT / "res" / "dumpcode_to_function.csv"
DISPLAY_LINE = re.compile(r"^\s*(\d{3})\s*[-\u2010-\u2015]\s*(.*)$")
DIGITS = re.compile(r"\d+")
RCL_OPERAND = re.compile(r"RCL\W*([0-9O])\b", re.IGNORECASE)

VISION_OCR = r"""
import Foundation
import Vision

let imageURL = URL(fileURLWithPath: CommandLine.arguments.last!)
func recognize(region: CGRect, source: String) throws -> [[String: Any]] {
    let request = VNRecognizeTextRequest()
    request.recognitionLevel = .accurate
    request.usesLanguageCorrection = false
    request.regionOfInterest = region
    try VNImageRequestHandler(url: imageURL).perform([request])
    return (request.results ?? []).compactMap { item in
        guard let text = item.topCandidates(1).first?.string else { return nil }
        return [
            "x": region.minX + item.boundingBox.midX * region.width,
            "y": region.minY + item.boundingBox.midY * region.height,
            "text": text,
            "source": source
        ]
    }
}
let displayObservations = try recognize(
    region: CGRect(x: 0.30, y: 0, width: 0.45, height: 1),
    source: "display"
)
let keyObservations = try recognize(
    region: CGRect(x: 0.02, y: 0.05, width: 0.18, height: 0.9),
    source: "keys"
)
let keyLinePattern = #"^\s*\d{3}\s*[-–—]"#
let lineKeyObservations = try displayObservations
    .filter {
        ($0["text"] as? String)?.range(
            of: keyLinePattern,
            options: .regularExpression
        ) != nil
    }
    .flatMap { item -> [[String: Any]] in
        guard let y = item["y"] as? CGFloat else { return [] }
        return try recognize(
            region: CGRect(x: 0.02, y: max(0, y - 0.02), width: 0.24, height: 0.04),
            source: "keys"
        )
    }
let observations = displayObservations + keyObservations + lineKeyObservations
let data = try JSONSerialization.data(withJSONObject: observations)
FileHandle.standardOutput.write(data)
"""


def read_instruction_names(path):
    names = {}
    with path.open(newline="", encoding="utf-8-sig") as csv_file:
        for row in csv.DictReader(csv_file):
            code = " ".join((row.get("Code") or "").split())
            label = (row.get("Label") or "").strip()
            if not code or not label:
                continue

            parts = [label, (row.get("Operator") or "").strip(),
                     (row.get("Alpha") or "").strip()]
            name = " ".join(part for part in parts if part)
            if code.startswith("42 "):
                name = f"f {name}"
            elif code.startswith("43 "):
                name = f"g {name}"
            names[code] = name
    return names


def recognize_text(image_path):
    result = subprocess.run(
        ["swift", "-e", VISION_OCR, str(image_path.resolve())],
        capture_output=True,
        check=True,
        text=True,
    )
    return json.loads(result.stdout)


def extract_program(observations, instruction_names):
    anchors = {}
    for item in observations:
        if item["source"] != "display":
            continue
        if not 0.30 <= item["x"] <= 0.62:
            continue
        match = DISPLAY_LINE.match(item["text"])
        if match:
            line_number = int(match.group(1))
            if line_number:
                anchors[line_number] = {
                    "y": item["y"],
                    "values": [(item["x"], match.group(2))],
                }

    if not anchors:
        raise ValueError("No numbered program instructions were recognized.")

    for item in observations:
        if item["source"] != "display":
            continue
        if not 0.43 <= item["x"] <= 0.75 or not DIGITS.search(item["text"]):
            continue
        if DISPLAY_LINE.match(item["text"]):
            continue
        nearest = min(
            anchors.items(),
            key=lambda anchor: abs(anchor[1]["y"] - item["y"]),
        )
        if abs(nearest[1]["y"] - item["y"]) <= 0.015:
            duplicate = any(
                text == item["text"] and abs(x - item["x"]) <= 0.01
                for x, text in nearest[1]["values"]
            )
            if not duplicate:
                nearest[1]["values"].append((item["x"], item["text"]))

    line_numbers = sorted(anchors)
    missing = [
        number
        for number in range(line_numbers[0], line_numbers[-1] + 1)
        if number not in anchors
    ]
    if missing:
        raise ValueError(
            "Could not read display line number(s): "
            + ", ".join(f"{number:03d}" for number in missing)
        )

    output = []
    for output_number, display_number in enumerate(line_numbers, start=1):
        row = anchors[display_number]
        codes = [
            code
            for _, text in sorted(row["values"], key=lambda value: value[0])
            for code in DIGITS.findall(text)
        ]
        if codes == ["45"]:
            operands = {
                match.group(1).upper()
                for item in observations
                if item["source"] == "keys"
                and abs(item["y"] - row["y"]) <= 0.015
                for match in RCL_OPERAND.finditer(
                    item["text"].upper().replace("O", "0")
                )
            }
            if len(operands) == 1:
                codes.extend(operands)
        code = " ".join(codes)
        if not code:
            raise ValueError(
                f"No instruction code recognized on display line {display_number:03d}."
            )
        name = instruction_names.get(code)
        if not name:
            raise ValueError(
                f"No instruction name found for code '{code}' "
                f"on display line {display_number:03d}."
            )
        output.append(
            f"{output_number:03d} {{{code.rjust(12)} }} {name}"
        )
    return output


def resolve_image_path(image_argument):
    image_path = Path(image_argument).expanduser()
    if not image_path.is_absolute():
        if image_path.parts and image_path.parts[0] == IMAGE_DIR.name:
            image_path = PROJECT_ROOT / image_path
        else:
            image_path = IMAGE_DIR / image_path

    image_path = image_path.resolve()
    if not image_path.is_relative_to(IMAGE_DIR.resolve()):
        raise ValueError(f"Image must be inside {IMAGE_DIR}.")
    if not image_path.is_file():
        raise FileNotFoundError(f"Image file not found: {image_path}")
    return image_path


def resolve_output_path(image_path):
    return OUTPUT_DIR / f"{image_path.stem}.15c"


def main():
    if len(sys.argv) < 2:
        print("Command line usage: filename")
        sys.exit(1)

    arg_file = sys.argv[1]

    if platform.system() != "Darwin":
        sys.exit("OCR uses Apple's Vision framework and requires macOS.")

    try:
        image_path = resolve_image_path(arg_file)
        output_path = resolve_output_path(image_path)
        instruction_names = read_instruction_names(INSTRUCTION_MAP)
        observations = recognize_text(image_path)
        program = extract_program(observations, instruction_names)
    except (FileNotFoundError, ValueError) as error:
        sys.exit(str(error))
    except subprocess.CalledProcessError as error:
        message = error.stderr.strip() or str(error)
        sys.exit(f"Vision OCR failed: {message}")
    except (json.JSONDecodeError, ValueError) as error:
        sys.exit(str(error))

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(program) + "\n", encoding="utf-8")
    print(f"Wrote {len(program)} instructions to {output_path}")


if __name__ == "__main__":
    main()
