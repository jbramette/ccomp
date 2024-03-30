#ifndef CCOMP_ARCH_HPP
#define CCOMP_ARCH_HPP


#include <unordered_map>
#include <string_view>


namespace ccomp::arch
{
	using opcode = uint16_t;

	enum operand_type
	{
		//
		// special registers
		//
		reg_ar = 1,
		reg_st,
		reg_dt,

		//
		// general 8-bit registers
		//
		reg_rx,

		//
		// immediate 8-bit value
		//
		imm8,

		//
		// indirect value (has to be immediate)
		//
		imm8_indirect
	};

	constexpr uint16_t make_operands_mask(std::initializer_list<operand_type> operand_types)
	{
		uint16_t mask = 0;
		uint16_t shift = 0;

		for (operand_type op : operand_types)
		{
			mask |= (op << shift);
			shift += 3;
		}

		return mask;
	}

	constexpr auto MASK_ADD_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_ADD_R8_I8 = make_operands_mask({ reg_rx, imm8 });
	constexpr auto MASK_SUB_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_SUBA_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_OR_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_AND_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_XOR_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_SHR_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_SHL_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_RDUMP_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_RLOAD_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_MOV_R8_I8 = make_operands_mask({ reg_rx, imm8 });
	constexpr auto MASK_MOV_AR_R8 = make_operands_mask({ reg_ar, reg_rx });
	constexpr auto MASK_ADD_AR_R8 = make_operands_mask({ reg_ar, reg_rx });
	constexpr auto MASK_MOV_DT_R8 = make_operands_mask({ reg_dt, reg_rx });
	constexpr auto MASK_MOV_ST_R8 = make_operands_mask({ reg_st, reg_rx });
	constexpr auto MASK_MOV_R8_DT = make_operands_mask({ reg_rx, reg_dt });
	constexpr auto MASK_DRAW_R8_R8_I8 = make_operands_mask({ reg_rx, reg_rx, imm8 });
	constexpr auto MASK_RAND_R8_I8 = make_operands_mask({ reg_rx, imm8 });
	constexpr auto MASK_BCD_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_WKEY_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_SKE_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_SKNE_R8 = make_operands_mask({ reg_rx });
	constexpr auto MASK_SE_R8_I8 = make_operands_mask({ reg_rx, imm8 });
	constexpr auto MASK_SNE_R8_I8 = make_operands_mask({ reg_rx, imm8 });
	constexpr auto MASK_SE_R8_R8 = make_operands_mask({ reg_rx, reg_rx });
	constexpr auto MASK_SNE_R8_R8 = make_operands_mask({ reg_rx, reg_rx });


    unsigned char operands_count(std::string_view mnemonic);
	unsigned char regname2regindex(std::string_view regname);

	opcode _8XY4(uint8_t reg_index1, uint8_t reg_index2);
	opcode _8XY5(uint8_t reg_index1, uint8_t reg_index2);
	opcode _7XNN(uint8_t reg_index, uint8_t imm);
}

#endif //CCOMP_ARCH_HPP
