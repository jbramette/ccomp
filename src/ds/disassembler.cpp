#include <chasm/ds/disassembler.hpp>
#include <chasm/ds/paths.hpp>
#include <chasm/options.hpp>
#include <chasm/arch.hpp>


namespace chasm::ds
{
	disassembler::disassembler(std::vector<uint8_t> from_bytes, arch::addr from_addr)
		: binary(std::move(from_bytes))
	{
		flow.path_push(from_addr);
		ds_path();
		ds_graph.insert_path(current_path());
		flow.path_pop();
	}

	analysis_path& disassembler::current_path()
	{
		return flow.analyzed_path();
	}

	disassembly_graph disassembler::get_graph()
	{
		return ds_graph;
	}

	void disassembler::ds_path()
	{
		while (!current_path().ended())
			ds_next_instruction();
	}

	void disassembler::ds_next_instruction()
	{
		///
		/// The paths manager only manipulates "in-memory" addresses, most of the time offset=0x200
		/// whereas the disassembler works with "disk" addresses, so offset 0x200 maps to address (file offset) 0 on disk
		///
		const arch::addr ip = current_path().addr_end() - options::arg<arch::addr>("relocate");

		if (ip + 1 >= binary.size() || ip >= binary.size())
			throw chasm_exception("Unexpected end of bytes while decoding instruction during disassembly at address 0x{:04X}", ip);

		const auto opcode = static_cast<arch::opcode>(binary[ip] << 8 | binary[ip + 1]);

		const auto n1 = static_cast<uint8_t>((opcode & 0xF000) >> 12);
		const auto n2 = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
		const auto n3 = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
		const auto n4 = static_cast<uint8_t>(opcode & 0x000F);
		const auto imm8 = static_cast<arch::imm>(opcode & 0x00FF);
		const auto addr = static_cast<arch::addr>(opcode & 0x0FFF);

		switch (n1)
		{
			case 0x0:
			{
				switch (opcode)
				{
					case 0x00E0: ds_cls(); return;
					case 0x00EE: ds_ret(); return;
					case 0x00FB: ds_scrr(); return;
					case 0x00FC: ds_scrl(); return;
					case 0x00FD: ds_exit(); return;
					case 0x00FE: ds_low(); return;
					case 0x00FF: ds_high(); return;

					default:
					{
						if (n3 == 0xC)
						{
							ds_scrd();
							return;
						}
					}
				}

				break;
			}

			case 0x1: ds_jmp(addr); return;
			case 0x2: ds_call(addr); return;
			case 0xA: ds_mov_ar_addr(addr); return;
			case 0xB: ds_jmp_indirect(addr); return;

			case 0x3: ds_se_r8_imm(n2, imm8); return;
			case 0x4: ds_sne_r8_imm(n2, imm8); return;
			case 0x6: ds_mov_r8_imm(n2, imm8); return;
			case 0x7: ds_add_r8_imm(n2, imm8); return;
			case 0xC: ds_rand_r8_imm(n2, imm8); return;

			case 0x5:
				if (n4 == 0)
				{
					ds_se_r8_r8(n2, n3);
					return;
				}

				break;

			case 0x8:
				switch (n4)
				{
					case 0x0: ds_mov_r8_r8(n2, n3); return;
					case 0x1: ds_or_r8_r8(n2, n3); return;
					case 0x2: ds_and_r8_r8(n2, n3); return;
					case 0x3: ds_xor_r8_r8(n2, n3); return;
					case 0x4: ds_add_r8_r8(n2, n3); return;
					case 0x5: ds_sub_r8_r8(n2, n3); return;
					case 0x6: ds_shl_r8_r8(n2, n3); return;
					case 0x7: ds_suba_r8_r8(n2, n3); return;
					case 0xE: ds_shr_r8_r8(n2, n3); return;

					default: break;
				}

				break;

			case 0x9:
				if (n4 == 0)
				{
					ds_sne_r8_r8(n2, n3);
					return;
				}

			case 0xE:
				if (imm8 == 0x9E)
				{
					ds_ske_r8(n2);
					return;
				}
				else if (imm8 == 0xA1)
				{
					ds_skne_r8(n2);
					return;
				}

			case 0xF:
				switch (imm8)
				{
					case 0x07: ds_mov_r8_dt(n2); return;
					case 0x15: ds_mov_dt_r8(n2); return;
					case 0x18: ds_mov_st_r8(n2); return;
					case 0x1E: ds_add_ar_r8(n2); return;
					case 0x29: ds_ldf_r8(n2); return;
					case 0x30: ds_ldfs_r8(n2); return;
					case 0x55: ds_rdump_r8(n2); return;
					case 0x65: ds_rload_r8(n2); return;
					case 0x75: ds_saverpl_r8(n2); return;
					case 0x85: ds_loadrpl_r8(n2); return;

					default: break;
				}

				break;

			case 0xD: ds_draw_r8_r8_imm(n2, n3, n4); return;

			default:
				break;
		}

		throw disassembly_exception::decoding_error(opcode, ip);
	}

	void disassembler::ds_cls()
	{
		emit(arch::instruction_id::CLS, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_ret()
	{
		emit(arch::instruction_id::RET, arch::operands_mask::MASK_NONE);

		current_path().mark_end();
	}

	void disassembler::ds_scrr()
	{
		emit(arch::instruction_id::SCRR, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_scrl()
	{
		emit(arch::instruction_id::SCRL, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_exit()
	{
		emit(arch::instruction_id::EXIT, arch::operands_mask::MASK_NONE);

		current_path().mark_end();
	}

	void disassembler::ds_low()
	{
		emit(arch::instruction_id::LOW, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_high()
	{
		emit(arch::instruction_id::HIGH, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_scrd()
	{
		emit(arch::instruction_id::SCRD, arch::operands_mask::MASK_NONE);
	}

	void disassembler::ds_call(arch::addr subroutine_addr)
	{
		emit(arch::instruction_id::CALL, arch::operands_mask::MASK_ADDR, subroutine_addr);

		if (flow.was_visited(subroutine_addr))
			return;

		flow.callstack_push(subroutine_addr);
		ds_path();
		ds_graph.insert_proc(flow.analyzed_procedure().to_procedure());
		flow.callstack_pop();
	}

	void disassembler::ds_jmp(arch::addr location)
	{
		emit(arch::instruction_id::JMP, arch::operands_mask::MASK_ADDR, location);

		current_path().mark_end();

		if (flow.was_visited(location))
			return;

		flow.path_push(location);
		ds_path();

		if (!flow.inside_procedure())
			ds_graph.insert_path(current_path());

		flow.path_pop();
	}

	void disassembler::ds_mov_ar_addr(arch::addr addr)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_AR_ADDR, addr);
	}

	void disassembler::ds_jmp_indirect(arch::addr offset)
	{
		emit(arch::instruction_id::JMP, arch::operands_mask::MASK_ADDR_REL, offset);

		current_path().mark_end();
	}

	void disassembler::ds_se_r8_imm(arch::reg reg, arch::imm imm)
	{
		emit(arch::instruction_id::SE, arch::operands_mask::MASK_R8_IMM, reg, imm);

		const arch::addr next1 = current_path().addr_end();
		const arch::addr next2 = current_path().addr_end() + sizeof(arch::opcode);

		flow.path_push(next1);
		ds_path();

		if (!flow.inside_procedure())
			ds_graph.insert_path(current_path());

		flow.path_pop();

		flow.path_push(next2);
		ds_path();

		if (!flow.inside_procedure())
			ds_graph.insert_path(current_path());

		flow.path_pop();
	}

	void disassembler::ds_sne_r8_imm(arch::reg reg, arch::imm imm)
	{
		emit(arch::instruction_id::SNE, arch::operands_mask::MASK_R8_IMM, reg, imm);
	}

	void disassembler::ds_mov_r8_imm(arch::reg reg, arch::imm imm)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_R8_IMM, reg, imm);
	}

	void disassembler::ds_add_r8_imm(arch::reg reg, arch::imm imm)
	{
		if (imm == 1)
			emit(arch::instruction_id::INC, arch::operands_mask::MASK_R8, reg);
		else
			emit(arch::instruction_id::ADD, arch::operands_mask::MASK_R8_IMM, reg, imm);
	}

	void disassembler::ds_rand_r8_imm(arch::reg reg, arch::imm imm)
	{
		emit(arch::instruction_id::RAND, arch::operands_mask::MASK_R8_IMM, reg, imm);
	}

	void disassembler::ds_se_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SE, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_mov_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_or_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::OR, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_and_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::AND, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_xor_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::XOR, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_add_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::ADD, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_sub_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SUB, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_shl_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SHL, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_suba_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SUBA, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_shr_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SHR, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_sne_r8_r8(arch::reg reg1, arch::reg reg2)
	{
		emit(arch::instruction_id::SNE, arch::operands_mask::MASK_R8_R8, reg1, reg2);
	}

	void disassembler::ds_mov_r8_dt(arch::reg reg)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_R8_DT, reg);
	}

	void disassembler::ds_mov_dt_r8(arch::reg reg)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_DT_R8, reg);
	}

	void disassembler::ds_mov_st_r8(arch::reg reg)
	{
		emit(arch::instruction_id::MOV, arch::operands_mask::MASK_ST_R8, reg);
	}

	void disassembler::ds_add_ar_r8(arch::reg reg)
	{
		emit(arch::instruction_id::ADD, arch::operands_mask::MASK_AR_R8, reg);
	}

	void disassembler::ds_ldf_r8(arch::reg reg)
	{
		emit(arch::instruction_id::LDF, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_ldfs_r8(arch::reg reg)
	{
		emit(arch::instruction_id::LDFS, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_rdump_r8(arch::reg reg)
	{
		emit(arch::instruction_id::RDUMP, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_rload_r8(arch::reg reg)
	{
		emit(arch::instruction_id::RLOAD, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_saverpl_r8(arch::reg reg)
	{
		emit(arch::instruction_id::SAVERPL, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_loadrpl_r8(arch::reg reg)
	{
		emit(arch::instruction_id::LOADRPL, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_ske_r8(arch::reg reg)
	{
		emit(arch::instruction_id::SKE, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_skne_r8(arch::reg reg)
	{
		emit(arch::instruction_id::SKNE, arch::operands_mask::MASK_R8, reg);
	}

	void disassembler::ds_draw_r8_r8_imm(arch::reg reg1, arch::reg reg2, arch::imm imm)
	{
		emit(arch::instruction_id::DRAW, arch::operands_mask::MASK_R8_R8_IMM, reg1, reg2, imm);
	}
}
