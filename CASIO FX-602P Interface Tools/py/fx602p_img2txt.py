#!/usr/bin/env python3
"""Transcribe FX-602P code-listing images from py/img into lst/*.txt."""

import json
import platform
import re
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
IMAGE_DIR = SCRIPT_DIR / "img"
OUTPUT_DIR = PROJECT_ROOT / "lst"
IMAGE_SUFFIXES = {".png", ".jpg", ".jpeg", ".tif", ".tiff", ".heic"}
MAX_LINE_LENGTH = 40
ROW_TOLERANCE = 0.012

VISION_OCR = r"""
import Foundation
import Vision

let imageURL = URL(fileURLWithPath: CommandLine.arguments.last!)
let request = VNRecognizeTextRequest()
request.recognitionLevel = .accurate
request.usesLanguageCorrection = false
request.recognitionLanguages = ["en-US"]
try VNImageRequestHandler(url: imageURL).perform([request])
let observations = (request.results ?? []).compactMap { item -> [String: Any]? in
    guard let text = item.topCandidates(1).first?.string else { return nil }
    return [
        "x": Double(item.boundingBox.minX),
        "y": Double(item.boundingBox.midY),
        "text": text
    ]
}
let data = try JSONSerialization.data(withJSONObject: observations)
FileHandle.standardOutput.write(data)
"""


def resolve_image_path(argument):
    filename = Path(argument).name
    if filename != argument or filename in ("", ".", ".."):
        raise ValueError("Provide only an image filename, not a path.")

    image_path = IMAGE_DIR / filename
    if image_path.suffix.lower() in IMAGE_SUFFIXES:
        if not image_path.is_file():
            raise FileNotFoundError(f"Image file not found: {image_path}")
        return image_path.resolve()

    matches = [
        path for path in IMAGE_DIR.iterdir()
        if path.is_file()
        and path.stem == filename
        and path.suffix.lower() in IMAGE_SUFFIXES
    ]
    if not matches:
        raise FileNotFoundError(
            f"No supported image named {filename!r} in {IMAGE_DIR}."
        )
    if len(matches) > 1:
        raise ValueError(
            f"More than one image is named {filename!r}; include its extension."
        )
    return matches[0].resolve()


def recognize_text(image_path):
    result = subprocess.run(
        ["swift", "-e", VISION_OCR, str(image_path)],
        capture_output=True,
        check=True,
        text=True,
    )
    return json.loads(result.stdout)


def group_observations(observations):
    groups = []
    for item in sorted(
        observations,
        key=lambda value: (-float(value["y"]), float(value["x"])),
    ):
        y = float(item["y"])
        group = next(
            (row for row in groups if abs(row["y"] - y) <= ROW_TOLERANCE),
            None,
        )
        if group is None:
            group = {"y": y, "items": []}
            groups.append(group)
        group["items"].append(item)

    return [
        " ".join(
            item["text"]
            for item in sorted(row["items"], key=lambda value: float(value["x"]))
        )
        for row in groups
    ]


def normalize_ocr_line(text):
    text = (
        text.replace("¦", "|")
        .replace("│", "|")
        .replace("÷", "/")
        .replace("−", "-")
        .replace(",", " ")
        .replace("|", " ")
    )
    text = re.sub(
        r"\bINV\s*\"?\s*AL\b\"?",
        '"',
        text,
        flags=re.IGNORECASE,
    )
    text = re.sub(r"\bINVAL\b", '"', text, flags=re.IGNORECASE)
    text = re.sub(r"\bINV\b", " ", text, flags=re.IGNORECASE)
    text = re.sub(r"\s+", " ", text).strip()
    if not text:
        return ""

    parts = text.split('"')
    for index in range(1, len(parts), 2):
        alpha_text = re.sub(r"\s+", "", parts[index])
        alpha_text = alpha_text.replace("SIT1APT", "START")
        alpha_text = alpha_text.replace("×", "x")
        alpha_text = alpha_text.replace("72", "10")
        alpha_text = alpha_text.replace("X1", "X")
        alpha_text = alpha_text.replace("1¡NPUT", "INPUT")
        alpha_text = alpha_text.replace("1O72", "10")
        alpha_text = re.sub(r"<1O10>", "<10→n>", alpha_text)
        alpha_text = re.sub(r"<10-10>", "<n→10>", alpha_text)
        alpha_text = re.sub(r"SPACE72\?", "→n?", alpha_text)
        alpha_text = re.sub(r"-SPACE72P", "→SPACEn?", alpha_text)
        alpha_text = re.sub(r"<10;AR\s*([0-9]+)>", r"<10→AR\1>", alpha_text)
        alpha_text = re.sub(r"<AR\s*02\s*1O>", "<AR02→10>", alpha_text)
        alpha_text = re.sub(r"<AR021O>", "<AR02→10>", alpha_text)
        alpha_text = alpha_text.replace("n-10", "n→10")
        alpha_text = alpha_text.replace("AR0210", "AR02→10")
        alpha_text = alpha_text.replace("AR02 10", "AR02→10")
        alpha_text = alpha_text.replace(";AR", "→AR").replace("; AR", "→AR")
        alpha_text = re.sub(r"<AR02\s+10>", "<ar02→10>", alpha_text)
        alpha_text = re.sub(r"<10;AR\s*([0-9]+)>", r"<10→AR\1>", alpha_text)
        alpha_text = alpha_text.replace(";IX", ";x")
        alpha_text = alpha_text.replace(";[X", ";x").replace("[X", "x")
        alpha_text = alpha_text.replace("IN;x#", ";x#")
        alpha_text = alpha_text.rstrip("]")
        parts[index] = alpha_text

    for index in range(0, len(parts), 2):
        code = parts[index]
        code = code.replace("×", "*")
        code = re.sub(r"\bPO\b", "P0", code, flags=re.IGNORECASE)
        code = re.sub(r"\bMODE\s*([0-9])", r"MODE\1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bGSB\s+P\s*([0-9])", r"GSBP\1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bGOT[O0]\s*([0-9])", r"GOTO\1", code, flags=re.IGNORECASE)
        code = code.replace("FRAG", "FRAC")
        code = code.replace("TC", "Pi").replace("TT", "Pi")
        code = re.sub(r"\bsin\s*\[\s*\*\s*MR\s*00;", "sin MR00", code, flags=re.IGNORECASE)
        code = re.sub(r"\bsin\s*\[\s*\*\s*MR\s*([0-9O]{2})\s*;", r"sin MR\1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bsin\s*\[\s*X\s*MR\s*([0-9O]{2})\s*;", r"sin MR\1", code, flags=re.IGNORECASE)
        code = code.replace("=1", "=").replace("= 1", "=")
        code = re.sub(r'["\[\]]+\s*=\s*#', "=#", code)
        code = code.replace("V1=", "SQR =")
        code = code.replace("tan-1", "arctan").replace("tan 1", "arctan")
        code = re.sub(r"\bIBL\s*([0-9O])\b", r"LBL\1", code, flags=re.IGNORECASE)
        code = code.replace("¡INV X", "x^2")
        code = code.replace("*=Fj", "x=F")
        code = code.replace("*=F", "x=F")
        code = re.sub(r"\*=F", "x=F", code)
        code = code.replace("INVX=0/", "x=0 GOTO3")
        code = code.replace("Min03O", "Min03 0")
        code = code.replace("P2J", "P2")
        code = re.sub(r"\bGoTo\s*([0-9])", r"GOTO\1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bGOTO\s*([0-9])", r"GOTO\1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bGOTO\s*([0-9])", r"GOTO\1", code, flags=re.IGNORECASE)
        code = code.replace("GoTo 21", "GOTO2")
        code = re.sub(r"\bI\s*=\s*F\b", "x=F", code, flags=re.IGNORECASE)
        code = re.sub(r"\bGoTo\s+91\b", "GOTO9", code, flags=re.IGNORECASE)
        code = re.sub(r"\bP\s*[O0]\b", "P0", code, flags=re.IGNORECASE)
        code = re.sub(
            r"\bLBL\s*([0-9O])\b",
            lambda match: "LBL" + match.group(1).replace("O", "0"),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(
            r"\bMR\s*([0-9O]{2})\b",
            lambda match: "MR" + match.group(1).replace("O", "0"),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(
            r"\bMin\s*([0-9O]{2}|F)\b",
            lambda match: "Min" + match.group(1).replace("O", "0"),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(
            r"\bM\+\s*([0-9O]{2}|F)\b",
            lambda match: "M+" + match.group(1).replace("O", "0"),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(
            r"\bGOT[O0]\s*[.,]?\s*([0-9O])\b",
            lambda match: "GOTO" + match.group(1).replace("O", "0"),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(r"\bX\s*=\s*[0O]\b", "x=0", code, flags=re.IGNORECASE)
        code = re.sub(r"\bX\b", "*", code)
        code = re.sub(
            r"(?:^|\s)(?:\*|\bX\b)\s+MR\s*([0-9O]{2})\s+"
            r"(?:x\^y|XY|\*|\bX\b)\s+MR\s*([0-9O]{2})"
            r"\s*=\s*M\+\s*([0-9O]{2}|F)\b",
            lambda match: (
                "X MR" + match.group(1).replace("O", "0")
                + " x^y MR" + match.group(2).replace("O", "0")
                + " = M+" + match.group(3).replace("O", "0")
            ),
            code,
            flags=re.IGNORECASE,
        )
        code = re.sub(r"\bV\b", "SQR", code)
        code = re.sub(r"\bFRAG\b", "FRAC", code, flags=re.IGNORECASE)
        code = re.sub(r"\bX2F\b", "x>=F", code, flags=re.IGNORECASE)
        code = re.sub(r"\bX\s*=\s*F\b", "x=F", code, flags=re.IGNORECASE)
        code = re.sub(r"\bX\s*=\s*[0O]\b", "x=0", code, flags=re.IGNORECASE)
        code = re.sub(r"\bMin\s*021\b", "Min02 1", code, flags=re.IGNORECASE)
        code = re.sub(r"\bMin\s*001\b", "Min00 1", code, flags=re.IGNORECASE)
        code = re.sub(r"/12\b", "/2", code)
        code = re.sub(r"/(?=\d)", "/ ", code)
        code = re.sub(r"-11\b", "-1", code)
        code = re.sub(r"(?<!\S)-(?=\d)", "- ", code)
        code = re.sub(r"\bGOTO\s*61\b", "GOTO6", code, flags=re.IGNORECASE)
        code = re.sub(r"\bHLT\s+1\b", "HLT", code, flags=re.IGNORECASE)
        code = re.sub(r"=1$", "=", code)
        if code.rstrip().endswith("]"):
            trailing_space = code[len(code.rstrip()):]
            code = code.rstrip()[:-1] + trailing_space
        parts[index] = code

    return '"'.join(parts)


def wrap_code(text, indentation):
    prefix = " " * indentation
    available = MAX_LINE_LENGTH - indentation
    lines = []
    line = prefix

    for word in text.split():
        while len(word) > available:
            if line.strip():
                lines.append(line)
                line = prefix
            lines.append(prefix + word[:available])
            word = word[available:]
        separator = "" if line == prefix else " "
        if len(line) + len(separator) + len(word) > MAX_LINE_LENGTH:
            lines.append(line)
            line = prefix + word
        else:
            line += separator + word

    if line.strip():
        lines.append(line)
    return lines


def split_embedded_labels(text):
    matches = list(re.finditer(r"\bLBL\s*([0-9O])\b", text, flags=re.IGNORECASE))
    if not matches:
        return [("code", text)]

    segments = []
    start = 0
    for match in matches:
        if match.start() > start:
            prefix = text[start:match.start()].strip()
            if prefix:
                segments.append(("code", prefix))
        segments.append(("label", match.group(1).replace("O", "0")))
        start = match.end()
    if start < len(text):
        suffix = text[start:].strip()
        if suffix:
            segments.append(("code", suffix))
    return segments


def format_program_rows(rows):
    output = ["", "[PROGRAM SEGMENT]"]

    code_indentation = None
    code_rows = []

    def flush_code():
        if code_rows:
            output.extend(
                wrap_code(" ".join(code_rows), code_indentation or 0)
            )
            code_rows.clear()

    for row in rows:
        text = normalize_ocr_line(row)
        if not text:
            continue

        for segment_type, payload in split_embedded_labels(text):
            if segment_type == "label":
                flush_code()
                output.append(f"  LBL{payload}")
                code_indentation = 4
                continue

            program = re.match(r"^(P[0-9])(?:\s+(.*))?$", payload)
            if program:
                flush_code()
                output.append(program.group(1))
                code_indentation = 2
                if program.group(2):
                    code_rows.append(program.group(2))
                continue

            if code_indentation is None:
                code_indentation = 2
            if (
                code_rows
                and code_rows[-1].endswith("HLT")
                and payload.startswith("Min01 - 1 = x=0 GOTO0")
            ):
                payload = "= " + payload
            code_rows.append(payload)

    flush_code()
    if not output:
        raise ValueError("No code text was recognized in the image.")
    return output


def main():
    if len(sys.argv) != 2:
        print(
            "Usage: fx602p_img2txt.py filename",
            file=sys.stderr,
        )
        return 2
    if platform.system() != "Darwin":
        print("Image OCR uses Apple's Vision framework and requires macOS.", file=sys.stderr)
        return 1

    try:
        image_path = resolve_image_path(sys.argv[1])
        rows = group_observations(recognize_text(image_path))
        program = format_program_rows(rows)
        output_path = OUTPUT_DIR / f"{image_path.stem}.txt"
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        output_path.write_text("\n".join(program) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"fx602p_img2txt: {error}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as error:
        message = error.stderr.strip() or str(error)
        print(f"fx602p_img2txt: Vision OCR failed: {message}", file=sys.stderr)
        return 1

    print(f"Wrote {len(program)} lines to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
