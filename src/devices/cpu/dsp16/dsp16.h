// license:BSD-3-Clause
// copyright-holders:Andrew Gardner
/***************************************************************************

    dsp16.h

    WE|AT&T DSP16 series emulator.

***************************************************************************/

#pragma once

#ifndef __DSP16_H__
#define __DSP16_H__


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> dsp16_device

class dsp16_device : public cpu_device
{
public:
	// construction/destruction
	dsp16_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// public interfaces

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_execute_interface overrides
	virtual uint64_t execute_clocks_to_cycles(uint64_t clocks) const override { return (clocks + 2 - 1) / 2; } // internal /2 divider
	virtual uint64_t execute_cycles_to_clocks(uint64_t cycles) const override { return (cycles * 2); } // internal /2 divider
	virtual uint32_t execute_min_cycles() const override;
	virtual uint32_t execute_max_cycles() const override;
	virtual uint32_t execute_input_lines() const override;
	virtual void execute_run() override;
	virtual void execute_set_input(int inputnum, int state) override;

	// device_memory_interface overrides
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const override;

	// device_state_interface overrides
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const override;

	// device_disasm_interface overrides
	virtual uint32_t disasm_min_opcode_bytes() const override;
	virtual uint32_t disasm_max_opcode_bytes() const override;
	virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const uint8_t *oprom, const uint8_t *opram, uint32_t options) override;

	// address spaces
	const address_space_config m_program_config;
	const address_space_config m_data_config;

	// CPU registers
	// ROM Address Arithmetic Unit (XAAU)  (page 2-4)
	uint16_t m_i;     // 12 bits
	uint16_t m_pc;
	uint16_t m_pt;
	uint16_t m_pr;
	uint16_t m_pi;

	// RAM Address Arithmetic Unit (YAAU)  (page 2-6)
	uint16_t m_j;     // Signed
	uint16_t m_k;     // Signed
	uint16_t m_rb;
	uint16_t m_re;
	uint16_t m_r0;
	uint16_t m_r1;
	uint16_t m_r2;
	uint16_t m_r3;

	// Data Arithmetic Unit (DAU)  (page 2-6)
	uint16_t m_x;
	uint32_t m_y;
	uint32_t m_p;
	uint64_t m_a0;    // 36 bits
	uint64_t m_a1;    // 36 bits
	uint8_t m_auc;    // 6 bits
	uint16_t m_psw;
	uint8_t m_c0;
	uint8_t m_c1;
	uint8_t m_c2;

	// Serial and parallel interfaces (TODO: More here  (page 2-13))
	uint16_t m_sioc;
	uint16_t m_srta;
	uint16_t m_sdx;
	uint16_t m_pioc;
	uint16_t m_pdx0;  // pdx0 & pdx1 refer to the same physical register (page 6-1)
	uint16_t m_pdx1;  // but we keep them separate for logic's sake.

	// internal stuff
	uint16_t m_ppc;

	// This CPU core handles the cache as more of a loop than 15 separate memory elements.
	// It's a bit of a hack, but it's easier this way (for now).
	uint16_t m_cacheStart;
	uint16_t m_cacheEnd;
	uint16_t m_cacheRedoNextPC;
	uint16_t m_cacheIterations;
	static const uint16_t CACHE_INVALID = 0xffff;

	// memory access
	inline uint32_t data_read(const uint16_t& addr);
	inline void data_write(const uint16_t& addr, const uint16_t& data);
	inline uint32_t opcode_read(const uint8_t pcOffset=0);

	// address spaces
	address_space* m_program;
	address_space* m_data;
	direct_read_data* m_direct;

	// other internal states
	int m_icount;

	// operations
	void execute_one(const uint16_t& op, uint8_t& cycles, uint8_t& pcAdvance);

	// table decoders
	void* registerFromRImmediateField(const uint8_t& R);
	void* registerFromRTable(const uint8_t& R);
	uint16_t* registerFromYFieldUpper(const uint8_t& Y);

	// execution
	void executeF1Field(const uint8_t& F1, const uint8_t& D, const uint8_t& S);
	void executeYFieldPost(const uint8_t& Y);
	void executeZFieldPartOne(const uint8_t& Z, uint16_t* rN);
	void executeZFieldPartTwo(const uint8_t& Z, uint16_t* rN);

	// helpers
	void* addressYL();
	void writeRegister(void* reg, const uint16_t& value);
	bool conditionTest(const uint8_t& CON);

	// flags
	bool lmi();
	bool leq();
	bool llv();
	bool lmv();
};


// device type definition
extern const device_type DSP16;


/***************************************************************************
    REGISTER ENUMERATION
***************************************************************************/

enum
{
	DSP16_I,        // ROM Address Arithmetic Unit (XAAU)
	DSP16_PC,
	DSP16_PT,
	DSP16_PR,
	DSP16_PI,
	DSP16_J,        // RAM Address Arithmetic Unit (YAAU)
	DSP16_K,
	DSP16_RB,
	DSP16_RE,
	DSP16_R0,
	DSP16_R1,
	DSP16_R2,
	DSP16_R3,
	DSP16_X,        // Data Arithmetic Unit (DAU)
	DSP16_Y,
	DSP16_P,
	DSP16_A0,
	DSP16_A1,
	DSP16_AUC,
	DSP16_PSW,
	DSP16_C0,
	DSP16_C1,
	DSP16_C2,
	DSP16_SIOC,
	DSP16_SRTA,
	DSP16_SDX,
	DSP16_PIOC,
	DSP16_PDX0,
	DSP16_PDX1
};


#endif /* __DSP16_H__ */
