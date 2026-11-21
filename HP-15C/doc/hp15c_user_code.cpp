// Copyright 2023 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <iomanip>
#include <iostream>
#include <vector>

#include "hp15c_user_code.h"

enum Digit_Interp
{
  Digit_NA,        // no values
  Digit_Basic,     // 0 .. 9
  Digit_2To9,      // 2 .. 9
  Digit_Dot,       // .0 .. .9
  Digit_User_Ind,  // 6 = user (i)
};


static std::map<int, std::string> digit_basic_map =
{
  { 0, "0" },
  { 1, "1" },
  { 2, "2" },
  { 3, "3" },
  { 4, "4" },
  { 5, "5" },
  { 6, "6" },
  { 7, "7" },
  { 8, "8" },
  { 9, "9" },
};

static std::map<int, std::string> digit_2to9_map =
{
  { 2, "2" },
  { 3, "3" },
  { 4, "4" },
  { 5, "5" },
  { 6, "6" },
  { 7, "7" },
  { 8, "8" },
  { 9, "9" },
};

static std::map<int, std::string> digit_dot_map =
{
  { 0, ".0" },
  { 1, ".1" },
  { 2, ".2" },
  { 3, ".3" },
  { 4, ".4" },
  { 5, ".5" },
  { 6, ".6" },
  { 7, ".7" },
  { 8, ".8" },
  { 9, ".9" },
};

static std::map<int, std::string> digit_user_ind_map =
{
  { 6, "u (i)" },
};

static std::map<int, std::map<int, std::string>> digit_map =
{
  { Digit_Basic,    digit_basic_map    },
  { Digit_2To9,     digit_2to9_map     },
  { Digit_Dot,      digit_dot_map      },
  { Digit_User_Ind, digit_user_ind_map },
};


enum Letter_Interp
{
  Letter_NA,       // no values
  Letter_Basic,    // A .. E
  Letter_MAT,      // MAT A .. MAT E
  Letter_User,     // user A .. user E
  Letter_G,        // g A .. g E
  Letter_G_Ind,    // D = g (i)
  Letter_Ind_I,    // D = (i), E = I
  Letter_I,        // E = I
};

static std::map<int, std::string> letter_basic_map =
{
  { 0xa, "A" },
  { 0xb, "B" },
  { 0xc, "C" },
  { 0xd, "D" },
  { 0xe, "E" },
};

static std::map<int, std::string> letter_mat_map =
{
  { 0xa, "MAT A" },
  { 0xb, "MAT B" },
  { 0xc, "MAT C" },
  { 0xd, "MAT D" },
  { 0xe, "MAT E" },
};

static std::map<int, std::string> letter_user_map =
{
  { 0xa, "u A" },
  { 0xb, "u B" },
  { 0xc, "u C" },
  { 0xd, "u D" },
  { 0xe, "u E" },
};

static std::map<int, std::string> letter_g_map =
{
  { 0xa, "g A" },
  { 0xb, "g B" },
  { 0xc, "g C" },
  { 0xd, "g D" },
  { 0xe, "g E" },
};

static std::map<int, std::string> letter_g_ind_map =
{
  { 0xd, "(i)" },
};

static std::map<int, std::string> letter_ind_i_map =
{
  { 0xd, "(i)" },
  { 0xe, "I" },
};

static std::map<int, std::string> letter_i_map =
{
  { 0xe, "I" },
};

static std::map<int, std::map<int, std::string>> letter_map =
{
  { Letter_Basic,   letter_basic_map   },
  { Letter_MAT,     letter_mat_map     },
  { Letter_User,    letter_user_map    },
  { Letter_G,       letter_g_map       },
  { Letter_G_Ind,   letter_g_ind_map   },
  { Letter_Ind_I,   letter_ind_i_map   },
  { Letter_I,       letter_i_map       },
};


struct Inst_Def
{
  uint16_t      opcode;
  std::string   mnemonic;
  Digit_Interp  interp_digit;
  Letter_Interp interp_letter;
};

static std::vector<Inst_Def> inst_def =
{
  { 0x00, "LBL",         Digit_Basic,    Letter_Basic },
  { 0x10, "GTO",         Digit_Basic,    Letter_Basic },
  { 0x20, "GSB",         Digit_Basic,    Letter_Basic },
  { 0x30, "RCL",         Digit_Basic,    Letter_Basic },
  { 0x40, "STO",         Digit_Basic,    Letter_Basic },
  { 0x50, "RCL",         Digit_Dot,      Letter_NA    },
  { 0x50, "RCL",         Digit_NA,       Letter_G     },
  { 0x60, "STO",         Digit_Dot,      Letter_NA    },
  { 0x60, "STO",         Digit_NA,       Letter_G     },
  { 0x70, "TEST",        Digit_Basic,    Letter_NA    },
  { 0x70, "RCL",         Digit_NA,       Letter_MAT   },

  { 0x80, "x⇄ 0",       Digit_NA,       Letter_NA    },
  { 0x81, "x⇄ (i)",     Digit_NA,       Letter_NA    },
  { 0x82, "DSE 0",       Digit_NA,       Letter_NA    },
  { 0x83, "DSE (i)",     Digit_NA,       Letter_NA    },
  { 0x84, "ISG 0",       Digit_NA,       Letter_NA    },
  { 0x85, "ISG (i)",     Digit_NA,       Letter_NA    },
  { 0x86, "RCL (i)",     Digit_NA,       Letter_NA    },
  { 0x87, "RCL I",       Digit_NA,       Letter_NA    },
  { 0x88, "GTO I",       Digit_NA,       Letter_NA    },
  { 0x89, "GSB I",       Digit_NA,       Letter_NA    },
  { 0x80, "RES",         Digit_NA,       Letter_Basic },

  { 0x90, "x⇄ 1",       Digit_NA,       Letter_NA    },
  { 0x91, "x⇄ I",       Digit_NA,       Letter_NA    },
  { 0x92, "DSE 1",       Digit_NA,       Letter_NA    },
  { 0x93, "DSE I",       Digit_NA,       Letter_NA    },
  { 0x94, "ISG 1",       Digit_NA,       Letter_NA    },
  { 0x95, "ISG I",       Digit_NA,       Letter_NA    },
  { 0x96, "STO (i)",     Digit_NA,       Letter_NA    },
  { 0x97, "STO I",       Digit_NA,       Letter_NA    },
  { 0x98, "DIM (i)",     Digit_NA,       Letter_NA    },
  { 0x99, "DIM I",       Digit_NA,       Letter_NA    },
  { 0x90, "DIM",         Digit_NA,       Letter_Basic },

  { 0xa0, "ŷ,r",         Digit_NA,       Letter_NA    },
  { 0xa1, "RAN",         Digit_NA,       Letter_NA    },
  { 0xa2, "CLx",         Digit_NA,       Letter_NA    },
  { 0xa3, "FRAC",        Digit_NA,       Letter_NA    },
  { 0xa4, "I",           Digit_NA,       Letter_NA    },
  { 0xa5, "CLR REG",     Digit_NA,       Letter_NA    },
  { 0xa6, "STO RES",     Digit_NA,       Letter_NA    },
  { 0xa7, "RCL Σ",       Digit_NA,       Letter_NA    },
  { 0xa8, "RCL DIM (i)", Digit_NA,       Letter_NA    },
  { 0xa9, "RCL DIM I",   Digit_NA,       Letter_NA    },
  { 0xa0, "RCL DIM",     Digit_NA,       Letter_Basic },
				         
  { 0xb0, "s",           Digit_NA,        Letter_NA    },
  { 0xb1, "LSTx",        Digit_NA,       Letter_NA    },
  { 0xb2, "RTN",         Digit_NA,       Letter_NA    },
  { 0xb3, "ABS",         Digit_NA,       Letter_NA    },
  { 0xb4, "R↑",          Digit_NA,       Letter_NA    },
  { 0xb5, "RND",         Digit_NA,       Letter_NA    },
  { 0xb6, "π",           Digit_NA,       Letter_NA    },
  { 0xb7, "SIN⁻¹",        Digit_NA,       Letter_NA    },
  { 0xb8, "COS⁻¹",        Digit_NA,       Letter_NA    },
  { 0xb9, "TAN⁻¹",        Digit_NA,       Letter_NA    },
  { 0xba, "x²",          Digit_NA,       Letter_NA    },
  { 0xbb, "LN",          Digit_NA,       Letter_NA    },
  { 0xbc, "LOG",         Digit_NA,       Letter_NA    },
  { 0xbd, "%",           Digit_NA,       Letter_NA    },
  { 0xbe, "Δ%",          Digit_NA,       Letter_NA    },
  
  { 0xc0, ".",           Digit_NA,       Letter_NA    },
  { 0xc1, "ENTER↑",       Digit_NA,       Letter_NA    },
  { 0xc2, "R/S",         Digit_NA,       Letter_NA    },
  { 0xc3, "CHS",         Digit_NA,       Letter_NA    },
  { 0xc4, "R↓",          Digit_NA,       Letter_NA    },
  { 0xc5, "x⇄y",        Digit_NA,       Letter_NA    },
  { 0xc6, "EEX",         Digit_NA,       Letter_NA    },
  { 0xc7, "SIN",         Digit_NA,       Letter_NA    },
  { 0xc8, "COS",         Digit_NA,       Letter_NA    },
  { 0xc9, "TAN",         Digit_NA,       Letter_NA    },
  { 0xca, "√x",          Digit_NA,       Letter_NA    },
  { 0xcb, "eᕁ",          Digit_NA,       Letter_NA    },
  { 0xcc, "10ᕁ",         Digit_NA,       Letter_NA    },
  { 0xcd, "yᕁ",          Digit_NA,       Letter_NA    },
  { 0xce, "1/x",         Digit_NA,       Letter_NA    },
  
  { 0xd0, "x!",          Digit_NA,       Letter_NA    },
  { 0xd1, "→R",         Digit_NA,        Letter_NA    },
  { 0xd2, "→H.MS",       Digit_NA,       Letter_NA    },
  { 0xd3, "→RAD",        Digit_NA,       Letter_NA    },
  { 0xd4, "HYP SIN",     Digit_NA,       Letter_NA    },
  { 0xd5, "HYP COS",     Digit_NA,       Letter_NA    },
  { 0xd6, "HYP TAN",     Digit_NA,       Letter_NA    },
  { 0xd7, "RCL RES",     Digit_NA,       Letter_NA    },
  { 0xd8, "RCL RAN",     Digit_NA,       Letter_NA    },
  { 0xd9, "STO RAN",     Digit_NA,       Letter_NA    },
  { 0xda, "Py,x",        Digit_NA,       Letter_NA    },
  { 0xdb, "Re⇄Im",      Digit_NA,       Letter_NA    },
  { 0xdc, "PSE",         Digit_NA,       Letter_NA    },
  { 0xdd, "CLEAR Σ",     Digit_NA,       Letter_NA    },
  { 0xde, "L.R.",        Digit_NA,       Letter_NA    },
  
  { 0xe0, "x̄",           Digit_NA,       Letter_NA    },
  { 0xe1, "→P",          Digit_NA,       Letter_NA    },
  { 0xe2, "→H",          Digit_NA,       Letter_NA    },
  { 0xe3, "→DEG",        Digit_NA,       Letter_NA    },
  { 0xe4, "HYP SIN⁻¹",   Digit_NA,       Letter_NA    },
  { 0xe5, "HYP COS⁻¹",   Digit_NA,       Letter_NA    },
  { 0xe6, "HYP TAN⁻¹",   Digit_NA,       Letter_NA    },
  { 0xe7, "DEG",         Digit_NA,       Letter_NA    },
  { 0xe8, "RAD",         Digit_NA,       Letter_NA    },
  { 0xe9, "GRD",         Digit_NA,       Letter_NA    },
  { 0xea, "Cy,x",        Digit_NA,       Letter_NA    },
  { 0xeb, "INT",         Digit_NA,       Letter_NA    },
  { 0xec, "x=0",         Digit_NA,       Letter_NA    },
  { 0xed, "x≤y",        Digit_NA,       Letter_NA    },
  { 0xee, "Σ-",          Digit_NA,       Letter_NA    },
  
  { 0xf0, "0",           Digit_NA,       Letter_NA    },
  { 0xf1, "1",           Digit_NA,       Letter_NA    },
  { 0xf2, "2",           Digit_NA,       Letter_NA    },
  { 0xf3, "3",           Digit_NA,       Letter_NA    },
  { 0xf4, "4",           Digit_NA,       Letter_NA    },
  { 0xf5, "5",           Digit_NA,       Letter_NA    },
  { 0xf6, "6",           Digit_NA,       Letter_NA    },
  { 0xf7, "7",           Digit_NA,       Letter_NA    },
  { 0xf8, "8",           Digit_NA,       Letter_NA    },
  { 0xf9, "9",           Digit_NA,       Letter_NA    },
  { 0xfa, "+",           Digit_NA,       Letter_NA    },
  { 0xfb, "-",           Digit_NA,       Letter_NA    },
  { 0xfc, "×",           Digit_NA,       Letter_NA    },
  { 0xfd, "÷",           Digit_NA,       Letter_NA    },
  { 0xfe, "Σ+",          Digit_NA,       Letter_NA    },
				         
  { 0xaf50, "RCL",       Digit_NA,       Letter_G_Ind },
  { 0xaf60, "STO",       Digit_NA,       Letter_G_Ind },
  { 0xbf30, "RCL",       Digit_NA,       Letter_User  },
  { 0xbf40, "STO",       Digit_NA,       Letter_User  },
  { 0xbf80, "RCL",       Digit_User_Ind, Letter_NA    },
  { 0xbf90, "STO",       Digit_User_Ind, Letter_NA    },
  { 0xcf80, "RCL +",     Digit_Basic,    Letter_Basic },
  { 0xcf90, "RCL +",     Digit_Dot,      Letter_Ind_I },
  { 0xcfa0, "RCL -",     Digit_Basic,    Letter_Basic },
  { 0xcfb0, "RCL -",     Digit_Dot,      Letter_Ind_I },
  { 0xcfc0, "RCL ×",     Digit_Basic,    Letter_Basic },
  { 0xcfd0, "RCL ×",     Digit_Dot,      Letter_Ind_I },
  { 0xcfe0, "RCL ÷",     Digit_Basic,    Letter_Basic },
  { 0xcff0, "RCL ÷",     Digit_Dot,      Letter_Ind_I },
  { 0xdf80, "STO +",     Digit_Basic,    Letter_Basic },
  { 0xdf90, "STO +",     Digit_Dot,      Letter_Ind_I },
  { 0xdfa0, "STO -",     Digit_Basic,    Letter_Basic },
  { 0xdfb0, "STO -",     Digit_Dot,      Letter_Ind_I },
  { 0xdfc0, "STO ×",     Digit_Basic,    Letter_Basic },
  { 0xdfd0, "STO ×",     Digit_Dot,      Letter_Ind_I },
  { 0xdfe0, "STO ÷",     Digit_Basic,    Letter_Basic },
  { 0xdff0, "STO ÷",     Digit_Dot,      Letter_Ind_I },
  { 0xef10, "SOLVE",     Digit_Basic,    Letter_Basic },
  { 0xef20, "SOLVE",     Digit_Dot,      Letter_NA    },
  { 0xef30, "INTEG",     Digit_Basic,    Letter_Basic },
  { 0xef40, "INTEG",     Digit_Dot,      Letter_NA    },
  { 0xef50, "x⇄",       Digit_2To9,     Letter_Basic },
  { 0xef60, "x⇄",       Digit_Dot,      Letter_NA    },
  { 0xef70, "DSE",       Digit_2To9,     Letter_Basic },
  { 0xef80, "DSE",       Digit_Dot,      Letter_NA    },
  { 0xef90, "ISG",       Digit_2To9,     Letter_Basic },
  { 0xefa0, "ISG",       Digit_Dot,      Letter_NA    },
  { 0xff00, "LBL",       Digit_Dot,      Letter_NA    },
  { 0xff10, "GTO",       Digit_Dot,      Letter_NA    },
  { 0xff20, "GSB",       Digit_Dot,      Letter_NA    },
  { 0xff30, "SF",        Digit_Basic,    Letter_I     },
  { 0xff40, "CF",        Digit_Basic,    Letter_I     },
  { 0xff50, "F?",        Digit_Basic,    Letter_I     },
  { 0xff60, "FIX",       Digit_Basic,    Letter_I     },
  { 0xff70, "SCI",       Digit_Basic,    Letter_I     },
  { 0xff80, "ENG",       Digit_Basic,    Letter_I     },
  { 0xff90, "MAT",       Digit_Basic,    Letter_NA    },
  { 0xffa0, "STO MAT",   Digit_NA,       Letter_Basic },
};



// keys 0x00 through 0xfe are one-byte instructions
//      least significant digit cannot be 0xf
// keys 0x0f00 through 0xfffe are two-byte instructions
//      least significant digit of high byte must be 0xf
//      least significant digit of low byte cannot be 0xf
std::map<uint16_t, std::string> HP15C_User_Code::inst_mnemonic;


HP15C_User_Code* HP15C_User_Code::instance = nullptr;

static HP15C_User_Code* instance = HP15C_User_Code::get_instance();


HP15C_User_Code* HP15C_User_Code::get_instance()
{
  if (! instance)
    instance = new HP15C_User_Code();
  return instance;
}


HP15C_User_Code::HP15C_User_Code()
{
  build_table();
}


void HP15C_User_Code::build_table()
{
  for (auto& inst: inst_def)
  {

    // std::cout << std::hex << inst.opcode << ": " << inst.mnemonic << "\n";

    if ((inst.interp_digit == Digit_NA) &&
	(inst.interp_letter == Letter_NA))
    {
      inst_mnemonic[inst.opcode] = inst.mnemonic;
    }
    else
    {
      if (inst.interp_digit != Digit_NA)
      {
	for (int i = 0; i <= 9; i++)
	{
	  if (digit_map[inst.interp_digit].contains(i))
	  {
	    inst_mnemonic[inst.opcode + i] = inst.mnemonic + " " + digit_map[inst.interp_digit][i];
	  }
	}
      }
      if (inst.interp_letter != Letter_NA)
      {
	for (int i = 0xa; i <= 0xe; i++)
	{
	  if (letter_map[inst.interp_letter].contains(i))
	  {
	    inst_mnemonic[inst.opcode + i] = inst.mnemonic + " " + letter_map[inst.interp_letter][i];
	  }
	}
      }
    }
  }
}

bool HP15C_User_Code::is_prefix(uint8_t byte)
{
  return (byte & 0x0f) == 0x0f;
}

std::string HP15C_User_Code::lookup(uint8_t single_byte)
{
  uint16_t key = single_byte;
  if (not inst_mnemonic.contains(key))
    return {};
  return inst_mnemonic[key];
}

std::string HP15C_User_Code::lookup(uint8_t first_byte, uint8_t second_byte)
{
  uint16_t key = (((uint16_t) first_byte) << 8) | second_byte;
  if (not inst_mnemonic.contains(key))
    return {};
  return inst_mnemonic[key];
}

void HP15C_User_Code::dump_table(std::ostream& out)
{
  for (auto& inst: inst_def)
  {
    out << std::hex << inst.opcode << ": " << inst.mnemonic << "\n";
  }
}

void HP15C_User_Code::dump_instructions(std::ostream& out)
{
  std::ios old_state(nullptr);
  old_state.copyfmt(out);

  int one_byte_count = 0;
  for (int i = 0x00; i <= 0xff; i++)
  {
    if (inst_mnemonic.contains(i))
    {
      std::string mnem = inst_mnemonic[i];
      out << std::hex << std::setfill('0') << std::setw(2) << "  " << i << ": " << mnem << "\n";
      one_byte_count++;
    }
  }
  int two_byte_count = 0;
  for (int i = 0x0100; i <= 0xffff; i++)
  {
    if (inst_mnemonic.contains(i))
    {
      std::string mnem = inst_mnemonic[i];
      out << std::hex << std::setfill('0') << std::setw(4) << i << ": " << mnem << "\n";
      two_byte_count++;
    }
  }

  out.copyfmt(old_state);

  out << "one byte instructions: " << one_byte_count << "\n";
  out << "two byte instructions: " << two_byte_count << "\n";
  out << "   total instructions: " << one_byte_count + two_byte_count << "\n";
}
