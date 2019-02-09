// license:BSD-3-Clause
// copyright-holders:David Haywood
/* Data East Backfire!

    Backfire!

    Note that the alternate "Potentiometer" controls (which set 2 has as the default) must first be calibrated by holding 1P Button 2 + Start on the "I/O Check" screen.

    The alternate "Optical Sensor" controls are not currently emulated.

    there may still be some problems with the 156 co-processor, but it seems to be mostly correct

*/

#include "emu.h"
#include "machine/adc0808.h"
#include "machine/decocrpt.h"
#include "machine/deco156.h"
#include "machine/eepromser.h"
#include "sound/okim6295.h"
#include "sound/ymz280b.h"
#include "cpu/arm/arm.h"
#include "video/deco16ic.h"
#include "video/decospr.h"
#include "emupal.h"
#include "rendlay.h"
#include "screen.h"
#include "speaker.h"


class backfire_state : public driver_device
{
public:
	backfire_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_mainram(*this, "mainram"),
		m_left_priority(*this, "left_priority"),
		m_right_priority(*this, "right_priority"),
		m_maincpu(*this, "maincpu"),
		m_sprgen(*this, "spritegen%u", 1U),
		m_deco_tilegen(*this, "tilegen%u", 1U),
		m_eeprom(*this, "eeprom"),
		m_adc(*this, "adc"),
		m_palette(*this, "palette"),
		m_lscreen(*this, "lscreen")
	{ }

	void backfire(machine_config &config);
	void init_backfire();

private:
	DECLARE_READ32_MEMBER(control2_r);
	template<int Layer> DECLARE_READ32_MEMBER(pf_rowscroll_r);
	template<int Layer> DECLARE_WRITE32_MEMBER(pf_rowscroll_w);
	template<int Chip> DECLARE_READ32_MEMBER(spriteram_r);
	template<int Chip> DECLARE_WRITE32_MEMBER(spriteram_w);
	DECLARE_READ32_MEMBER(backfire_speedup_r);
	DECLARE_WRITE8_MEMBER(eeprom_w);
	DECLARE_READ32_MEMBER(pot_select_r);
	virtual void machine_start() override;
	virtual void video_start() override;
	uint32_t screen_update_left(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update_right(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_WRITE_LINE_MEMBER(vbl_interrupt);
	DECLARE_WRITE32_MEMBER(irq_ack_w);
	void descramble_sound();
	DECO16IC_BANK_CB_MEMBER(bank_callback);
	DECOSPR_PRIORITY_CB_MEMBER(pri_callback);

	void backfire_map(address_map &map);

	/* memory pointers */
	std::unique_ptr<uint16_t[]>  m_spriteram[2];
	std::unique_ptr<uint16_t[]>  m_pf_rowscroll[4];
	required_shared_ptr<uint32_t> m_mainram;
	required_shared_ptr<uint32_t> m_left_priority;
	required_shared_ptr<uint32_t> m_right_priority;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device_array<decospr_device, 2> m_sprgen;
	required_device_array<deco16ic_device, 2> m_deco_tilegen;

	required_device<eeprom_serial_93cxx_device> m_eeprom;
	required_device<adc0808_device> m_adc;

	required_device<palette_device> m_palette;
	required_device<screen_device> m_lscreen;
};

//uint32_t *backfire_180010, *backfire_188010;

/* I'm using the functions in deco16ic.c ... same chips, why duplicate code? */
void backfire_state::video_start()
{
	m_spriteram[0] = std::make_unique<uint16_t[]>(0x2000/4);
	m_spriteram[1] = std::make_unique<uint16_t[]>(0x2000/4);

	/* and register the allocated ram so that save states still work */
	m_pf_rowscroll[0] = std::make_unique<uint16_t[]>(0x1000/4);
	m_pf_rowscroll[1] = std::make_unique<uint16_t[]>(0x1000/4);
	m_pf_rowscroll[2] = std::make_unique<uint16_t[]>(0x1000/4);
	m_pf_rowscroll[3] = std::make_unique<uint16_t[]>(0x1000/4);

	save_pointer(NAME(m_spriteram[0]), 0x2000/4);
	save_pointer(NAME(m_spriteram[1]), 0x2000/4);

	save_pointer(NAME(m_pf_rowscroll[0]), 0x1000/4);
	save_pointer(NAME(m_pf_rowscroll[1]), 0x1000/4);
	save_pointer(NAME(m_pf_rowscroll[2]), 0x1000/4);
	save_pointer(NAME(m_pf_rowscroll[3]), 0x1000/4);
}



uint32_t backfire_state::screen_update_left(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// sprites are flipped relative to tilemaps
	m_sprgen[0]->set_flip_screen(true);

	/* screen 1 uses pf1 as the forground and pf3 as the background */
	/* screen 2 uses pf2 as the foreground and pf4 as the background */
	m_deco_tilegen[0]->pf_update(m_pf_rowscroll[0].get(), m_pf_rowscroll[1].get());
	m_deco_tilegen[1]->pf_update(m_pf_rowscroll[2].get(), m_pf_rowscroll[3].get());

	screen.priority().fill(0);
	bitmap.fill(0x100, cliprect);

	if (m_left_priority[0] == 0)
	{
		m_deco_tilegen[1]->tilemap_1_draw(screen, bitmap, cliprect, 0, 1);
		m_deco_tilegen[0]->tilemap_1_draw(screen, bitmap, cliprect, 0, 2);
		m_sprgen[0]->draw_sprites(bitmap, cliprect, m_spriteram[0].get(), 0x2000/4);
	}
	else if (m_left_priority[0] == 2)
	{
		m_deco_tilegen[0]->tilemap_1_draw(screen, bitmap, cliprect, 0, 2);
		m_deco_tilegen[1]->tilemap_1_draw(screen, bitmap, cliprect, 0, 4);
		m_sprgen[0]->draw_sprites(bitmap, cliprect, m_spriteram[0].get(), 0x2000/4);
	}
	else
		popmessage( "unknown left priority %08x", m_left_priority[0]);

	return 0;
}

uint32_t backfire_state::screen_update_right(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// sprites are flipped relative to tilemaps
	m_sprgen[1]->set_flip_screen(true);

	/* screen 1 uses pf1 as the forground and pf3 as the background */
	/* screen 2 uses pf2 as the foreground and pf4 as the background */
	m_deco_tilegen[0]->pf_update(m_pf_rowscroll[0].get(), m_pf_rowscroll[1].get());
	m_deco_tilegen[1]->pf_update(m_pf_rowscroll[2].get(), m_pf_rowscroll[3].get());

	screen.priority().fill(0);
	bitmap.fill(0x500, cliprect);

	if (m_right_priority[0] == 0)
	{
		m_deco_tilegen[1]->tilemap_2_draw(screen, bitmap, cliprect, 0, 1);
		m_deco_tilegen[0]->tilemap_2_draw(screen, bitmap, cliprect, 0, 2);
		m_sprgen[1]->draw_sprites(bitmap, cliprect, m_spriteram[1].get(), 0x2000/4);
	}
	else if (m_right_priority[0] == 2)
	{
		m_deco_tilegen[0]->tilemap_2_draw(screen, bitmap, cliprect, 0, 2);
		m_deco_tilegen[1]->tilemap_2_draw(screen, bitmap, cliprect, 0, 4);
		m_sprgen[1]->draw_sprites(bitmap, cliprect, m_spriteram[1].get(), 0x2000/4);
	}
	else
		popmessage( "unknown right priority %08x", m_right_priority[0]);

	return 0;
}



WRITE8_MEMBER(backfire_state::eeprom_w)
{
	m_eeprom->clk_write(BIT(data, 1) ? ASSERT_LINE : CLEAR_LINE);
	m_eeprom->di_write(BIT(data, 0));
	m_eeprom->cs_write(BIT(data, 2) ? ASSERT_LINE : CLEAR_LINE);
}


/* map 32-bit writes to 16-bit */

template<int Layer> READ32_MEMBER(backfire_state::pf_rowscroll_r){ return m_pf_rowscroll[Layer][offset] ^ 0xffff0000; }
template<int Layer> WRITE32_MEMBER(backfire_state::pf_rowscroll_w){ data &= 0x0000ffff; mem_mask &= 0x0000ffff; COMBINE_DATA(&m_pf_rowscroll[Layer][offset]); }


READ32_MEMBER(backfire_state::pot_select_r)
{
	if (!machine().side_effects_disabled())
		m_adc->address_offset_start_w(offset, 0);
	return 0;
}

template<int Chip>
READ32_MEMBER(backfire_state::spriteram_r)
{
	return m_spriteram[Chip][offset] ^ 0xffff0000;
}

template<int Chip>
WRITE32_MEMBER(backfire_state::spriteram_w)
{
	data &= 0x0000ffff;
	mem_mask &= 0x0000ffff;

	COMBINE_DATA(&m_spriteram[Chip][offset]);
}


void backfire_state::backfire_map(address_map &map)
{
	map(0x000000, 0x0fffff).rom();
	map(0x100000, 0x10001f).rw(m_deco_tilegen[0], FUNC(deco16ic_device::pf_control_dword_r), FUNC(deco16ic_device::pf_control_dword_w));
	map(0x110000, 0x111fff).rw(m_deco_tilegen[0], FUNC(deco16ic_device::pf1_data_dword_r), FUNC(deco16ic_device::pf1_data_dword_w));
	map(0x114000, 0x115fff).rw(m_deco_tilegen[0], FUNC(deco16ic_device::pf2_data_dword_r), FUNC(deco16ic_device::pf2_data_dword_w));
	map(0x120000, 0x120fff).rw(FUNC(backfire_state::pf_rowscroll_r<0>), FUNC(backfire_state::pf_rowscroll_w<0>));
	map(0x124000, 0x124fff).rw(FUNC(backfire_state::pf_rowscroll_r<1>), FUNC(backfire_state::pf_rowscroll_w<1>));
	map(0x130000, 0x13001f).rw(m_deco_tilegen[1], FUNC(deco16ic_device::pf_control_dword_r), FUNC(deco16ic_device::pf_control_dword_w));
	map(0x140000, 0x141fff).rw(m_deco_tilegen[1], FUNC(deco16ic_device::pf1_data_dword_r), FUNC(deco16ic_device::pf1_data_dword_w));
	map(0x144000, 0x145fff).rw(m_deco_tilegen[1], FUNC(deco16ic_device::pf2_data_dword_r), FUNC(deco16ic_device::pf2_data_dword_w));
	map(0x150000, 0x150fff).rw(FUNC(backfire_state::pf_rowscroll_r<2>), FUNC(backfire_state::pf_rowscroll_w<2>));
	map(0x154000, 0x154fff).rw(FUNC(backfire_state::pf_rowscroll_r<3>), FUNC(backfire_state::pf_rowscroll_w<3>));
	map(0x160000, 0x161fff).rw(m_palette, FUNC(palette_device::read16), FUNC(palette_device::write16)).umask32(0x0000ffff).share("palette");
	map(0x170000, 0x177fff).ram().share("mainram");// main ram
	map(0x180010, 0x180013).nopw(); // always 180010 ?
	map(0x184000, 0x185fff).rw(FUNC(backfire_state::spriteram_r<0>), FUNC(backfire_state::spriteram_w<0>));
	map(0x188010, 0x188013).nopw(); // always 188010 ?
	map(0x18c000, 0x18dfff).rw(FUNC(backfire_state::spriteram_r<1>), FUNC(backfire_state::spriteram_w<1>));
	map(0x190000, 0x190003).portr("IN0");
	map(0x194000, 0x194003).portr("IN1");
	map(0x1a4000, 0x1a4000).w(FUNC(backfire_state::eeprom_w));
	map(0x1a8000, 0x1a8003).ram().share("left_priority");
	map(0x1ac000, 0x1ac003).ram().share("right_priority");
	map(0x1b0000, 0x1b0003).w(FUNC(backfire_state::irq_ack_w));
	map(0x1c0000, 0x1c0007).rw("ymz", FUNC(ymz280b_device::read), FUNC(ymz280b_device::write)).umask32(0x000000ff);
	map(0x1e4000, 0x1e4000).r("adc", FUNC(adc0808_device::data_r));
	map(0x1e8000, 0x1e8007).r(FUNC(backfire_state::pot_select_r));
}


static INPUT_PORTS_START( backfire )
	PORT_START("IN0")
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0000ff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x00080000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("lscreen")
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_UNUSED ) /* 'soundmask' */
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("lscreen")
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_93cxx_device, do_read)
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("adc", adc0808_device, eoc_r)
	PORT_BIT( 0xf8000000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN1")
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0000ff00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0xfff80000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PADDLE0")
	PORT_BIT ( 0xff, 0x80, IPT_PADDLE ) PORT_PLAYER(1) PORT_MINMAX(0x20, 0xe0) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START("PADDLE1")
	PORT_BIT ( 0xff, 0x80, IPT_PADDLE ) PORT_PLAYER(2) PORT_MINMAX(0x20, 0xe0) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_REVERSE
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ STEP8(0,1) },
	{ STEP8(0,8*2) },
	8*8*2    /* every char takes 8 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ STEP8(16*8*2,1), STEP8(0,1) },
	{ STEP16(0,8*2) },
	16*16*2
};


static GFXDECODE_START( gfx_backfire )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout,     0, 128 )   /* Characters 8x8 */
	GFXDECODE_ENTRY( "gfx1", 0, tilelayout,     0, 128 )   /* Tiles 16x16 */
	GFXDECODE_ENTRY( "gfx2", 0, charlayout,     0, 128 )   /* Characters 8x8 */
	GFXDECODE_ENTRY( "gfx2", 0, tilelayout,     0, 128 )   /* Tiles 16x16 */
	GFXDECODE_ENTRY( "gfx3", 0, tilelayout, 0x200,  32 )   /* Sprites 16x16 (screen 1) */
	GFXDECODE_ENTRY( "gfx4", 0, tilelayout, 0x600,  32 )   /* Sprites 16x16 (screen 2) */
GFXDECODE_END


WRITE_LINE_MEMBER(backfire_state::vbl_interrupt)
{
	if (state)
		m_maincpu->set_input_line(ARM_IRQ_LINE, ASSERT_LINE);
}

WRITE32_MEMBER(backfire_state::irq_ack_w)
{
	m_maincpu->set_input_line(ARM_IRQ_LINE, CLEAR_LINE);
}


DECO16IC_BANK_CB_MEMBER(backfire_state::bank_callback)
{
	//  osd_printf_debug("bank callback %04x\n",bank); // bit 1 gets set too?
	bank = bank >> 4;
	bank = (bank & 1) | ((bank & 4) >> 1) | ((bank & 2) << 1);

	return bank * 0x1000;
}

DECOSPR_PRIORITY_CB_MEMBER(backfire_state::pri_callback)
{
	switch (pri & 0xc000)
	{
		case 0x0000: return 0;    // numbers, people, cars when in the air, status display..
		case 0x4000: return 0xf0; // cars most of the time
		case 0x8000: return 0;    // car wheels during jump?
		case 0xc000: return 0xf0; // car wheels in race?
	}
	return 0;
}

void backfire_state::machine_start()
{
}

MACHINE_CONFIG_START(backfire_state::backfire)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", ARM, 28000000/4) /* Unconfirmed */
	MCFG_DEVICE_PROGRAM_MAP(backfire_map)

	EEPROM_93C46_16BIT(config, "eeprom");

	ADC0808(config, m_adc, 1000000); // unknown clock
	m_adc->in_callback<0>().set_ioport("PADDLE0");
	m_adc->in_callback<1>().set_ioport("PADDLE1");

	/* video hardware */
	PALETTE(config, m_palette).set_format(palette_device::xBGR_555, 2048);

	GFXDECODE(config, "gfxdecode", m_palette, gfx_backfire);
	config.set_default_layout(layout_dualhsxs);

	MCFG_SCREEN_ADD("lscreen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(backfire_state, screen_update_left)
	MCFG_SCREEN_PALETTE(m_palette)
	MCFG_SCREEN_VBLANK_CALLBACK(WRITELINE(*this, backfire_state, vbl_interrupt))

	MCFG_SCREEN_ADD("rscreen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(backfire_state, screen_update_right)
	MCFG_SCREEN_PALETTE(m_palette)

	DECO16IC(config, m_deco_tilegen[0], 0);
	m_deco_tilegen[0]->set_screen(m_lscreen);
	m_deco_tilegen[0]->set_split(0);
	m_deco_tilegen[0]->set_pf1_size(DECO_64x32);
	m_deco_tilegen[0]->set_pf2_size(DECO_64x32);
	m_deco_tilegen[0]->set_pf1_trans_mask(0x0f);
	m_deco_tilegen[0]->set_pf2_trans_mask(0x0f);
	m_deco_tilegen[0]->set_pf1_col_bank(0x00);
	m_deco_tilegen[0]->set_pf2_col_bank(0x40);
	m_deco_tilegen[0]->set_pf1_col_mask(0x0f);
	m_deco_tilegen[0]->set_pf2_col_mask(0x0f);
	m_deco_tilegen[0]->set_bank1_callback(FUNC(backfire_state::bank_callback), this);
	m_deco_tilegen[0]->set_bank2_callback(FUNC(backfire_state::bank_callback), this);
	m_deco_tilegen[0]->set_pf12_8x8_bank(0);
	m_deco_tilegen[0]->set_pf12_16x16_bank(1);
	m_deco_tilegen[0]->set_gfxdecode_tag("gfxdecode");

	DECO16IC(config, m_deco_tilegen[1], 0);
	m_deco_tilegen[1]->set_screen(m_lscreen);
	m_deco_tilegen[1]->set_split(0);
	m_deco_tilegen[1]->set_pf1_size(DECO_64x32);
	m_deco_tilegen[1]->set_pf2_size(DECO_64x32);
	m_deco_tilegen[1]->set_pf1_trans_mask(0x0f);
	m_deco_tilegen[1]->set_pf2_trans_mask(0x0f);
	m_deco_tilegen[1]->set_pf1_col_bank(0x10);
	m_deco_tilegen[1]->set_pf2_col_bank(0x50);
	m_deco_tilegen[1]->set_pf1_col_mask(0x0f);
	m_deco_tilegen[1]->set_pf2_col_mask(0x0f);
	m_deco_tilegen[1]->set_bank1_callback(FUNC(backfire_state::bank_callback), this);
	m_deco_tilegen[1]->set_bank2_callback(FUNC(backfire_state::bank_callback), this);
	m_deco_tilegen[1]->set_pf12_8x8_bank(2);
	m_deco_tilegen[1]->set_pf12_16x16_bank(3);
	m_deco_tilegen[1]->set_gfxdecode_tag("gfxdecode");

	DECO_SPRITE(config, m_sprgen[0], 0);
	m_sprgen[0]->set_screen(m_lscreen);
	m_sprgen[0]->set_gfx_region(4);
	m_sprgen[0]->set_pri_callback(FUNC(backfire_state::pri_callback), this);
	m_sprgen[0]->set_gfxdecode_tag("gfxdecode");

	DECO_SPRITE(config, m_sprgen[1], 0);
	m_sprgen[1]->set_screen("rscreen");
	m_sprgen[1]->set_gfx_region(5);
	m_sprgen[1]->set_pri_callback(FUNC(backfire_state::pri_callback), this);
	m_sprgen[1]->set_gfxdecode_tag("gfxdecode");

	/* sound hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	MCFG_DEVICE_ADD("ymz", YMZ280B, 28000000 / 2)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END


/*

Backfire!
Data East, 1995

This game is similar to World Rally, Blomby Car, Drift Out'94 etc


PCB Layout
----------


DE-0432-2
---------------------------------------------------------------------
|              MBZ-06.19L     28.000MHz                MBZ-04.19A * |
|                                           52                      |
|                              153                     MBZ-03.18A + |
|              MBZ-05.17L                                           |
|                                                                   |
--|        LC7881  YMZ280B-F   153          52         MBZ-04.16A * |
  |                                                                 |
--|                                                    MBZ-03.15A + |
|                     CY7C185 (x2)                                  |
|J                                     141                          |
|                                                      MBZ-02.12A   |
|A                                                                  |
|                                                      MBZ-01.10A   |
|M       223                                                        |
|                                                      MBZ-00.9A    |
|M         93C45.8M   CY7C185 (x2)     141                          |
|                                                                   |
|A                                                                  |
|                                                                   |
--|                                                                 |
  |                                                                 |
--|        TSW1                                                     |
|                                          CY7C185 (x4)             |
|                                                           156     |
|                 ADC0808       RA01-0.3J                           |
|                               RA00-0.2J                           |
|CONN2      CONN1    D4701                                          |
|                                                                   |
---------------------------------------------------------------------


Notes:
CONN1 & CONN2: For connection of potentiometer or opto steering wheel.
               Joystick (via JAMMA) can also be used for controls.
TSW1: Push Button TEST switch to access options menu (coins/lives etc).
*   : These ROMs have identical contents AND identical halves.
+   : These ROMs have identical contents AND identical halves.

*/

ROM_START( backfire )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "ra00-0.2j",    0x000002, 0x080000, CRC(790da069) SHA1(84fd90fb1833b97459cb337fdb92f7b6e93b5936) )
	ROM_LOAD32_WORD( "ra01-0.3j",    0x000000, 0x080000, CRC(447cb57b) SHA1(1d503b9cf1cadd3fdd7c9d6d59d4c40a59fa25ab))

	ROM_REGION( 0x400000, "gfx1", 0 ) /* Tiles 1 */
	ROM_LOAD( "mbz-00.9a",    0x000000, 0x080000, CRC(1098d504) SHA1(1fecd26b92faffce0b59a8a9646bfd457c17c87c) )
	ROM_CONTINUE( 0x200000, 0x080000)
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x300000, 0x080000)
	ROM_LOAD( "mbz-01.10a",    0x080000, 0x080000, CRC(19b81e5c) SHA1(4c8204a6a4ad30b23fbfdd79c6e39581e23de6ae) )
	ROM_CONTINUE( 0x280000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)
	ROM_CONTINUE( 0x380000, 0x080000)

	ROM_REGION( 0x100000, "gfx2", 0 ) /* Tiles 2 */
	ROM_LOAD( "mbz-02.12a",    0x000000, 0x100000, CRC(2bd2b0a1) SHA1(8fcb37728f3248ad55e48f2d398b014b36c9ec05) )

	ROM_REGION( 0x400000, "gfx3", 0 ) /* Sprites 1 */
	ROM_LOAD( "mbz-03.15a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD( "mbz-04.16a",    0x200000, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x400000, "gfx4", 0 ) /* Sprites 2 */
	ROM_LOAD( "mbz-03.18a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD( "mbz-04.19a",    0x200000, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x400000, "ymz", ROMREGION_ERASEFF ) /* samples */
	ROM_LOAD( "mbz-05.17l",    0x000000, 0x200000,  CRC(947c1da6) SHA1(ac36006e04dc5e3990f76539763cc76facd08376) )
	ROM_LOAD( "mbz-06.19l",    0x200000, 0x080000,  CRC(4a38c635) SHA1(7f0fb6a7a4aa6774c04fa38e53ceff8744fe1e9f) )

	ROM_REGION( 0x0600, "plds", 0 )
	ROM_LOAD( "gal16v8b.6b",  0x0000, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "gal16v8b.6d",  0x0200, 0x0117, NO_DUMP ) /* PAL is read protected */
	ROM_LOAD( "gal16v8b.12n", 0x0400, 0x0117, NO_DUMP ) /* PAL is read protected */
ROM_END

// different test mode font color
ROM_START( backfirea )
	ROM_REGION( 0x100000, "maincpu", 0 ) /* DE156 code (encrypted) */
	ROM_LOAD32_WORD( "rb-00h.h2",    0x000002, 0x080000, CRC(60973046) SHA1(e70d9be9cb172920da2a2ac9d317768b1438c59d) )
	ROM_LOAD32_WORD( "rb-01l.h3",    0x000000, 0x080000, CRC(27472f60) SHA1(d73b1e68dc51e28b1148db39ce22bd2e93f6fd0a) )

	ROM_REGION( 0x400000, "gfx1", 0 ) /* Tiles 1 */
	ROM_LOAD( "mbz-00.9a",    0x000000, 0x080000, CRC(1098d504) SHA1(1fecd26b92faffce0b59a8a9646bfd457c17c87c) )
	ROM_CONTINUE( 0x200000, 0x080000)
	ROM_CONTINUE( 0x100000, 0x080000)
	ROM_CONTINUE( 0x300000, 0x080000)
	ROM_LOAD( "mbz-01.10a",    0x080000, 0x080000, CRC(19b81e5c) SHA1(4c8204a6a4ad30b23fbfdd79c6e39581e23de6ae) )
	ROM_CONTINUE( 0x280000, 0x080000)
	ROM_CONTINUE( 0x180000, 0x080000)
	ROM_CONTINUE( 0x380000, 0x080000)

	ROM_REGION( 0x100000, "gfx2", 0 ) /* Tiles 2 */
	ROM_LOAD( "mbz-02.12a",    0x000000, 0x100000, CRC(2bd2b0a1) SHA1(8fcb37728f3248ad55e48f2d398b014b36c9ec05) )

	ROM_REGION( 0x400000, "gfx3", 0 ) /* Sprites 1 */
	ROM_LOAD( "mbz-03.15a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD( "mbz-04.16a",    0x200000, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x400000, "gfx4", 0 ) /* Sprites 2 */
	ROM_LOAD( "mbz-03.18a",    0x000000, 0x200000, CRC(2e818569) SHA1(457c1cad25d9b21459262be8b5788969f566a996) )
	ROM_LOAD( "mbz-04.19a",    0x200000, 0x200000, CRC(67bdafb1) SHA1(9729c18f3153e4bba703a6f46ad0b886c52d84e2) )

	ROM_REGION( 0x400000, "ymz", ROMREGION_ERASEFF ) /* samples */
	ROM_LOAD( "mbz-05.17l",    0x000000, 0x200000,  CRC(947c1da6) SHA1(ac36006e04dc5e3990f76539763cc76facd08376) )
	ROM_LOAD( "mbz-06.19l",    0x200000, 0x080000,  CRC(4a38c635) SHA1(7f0fb6a7a4aa6774c04fa38e53ceff8744fe1e9f) )
ROM_END

void backfire_state::descramble_sound()
{
	uint8_t *rom = memregion("ymz")->base();
	int length = 0x200000; // only the first rom is swapped on backfire!
	std::vector<uint8_t> buf1(length);
	uint32_t x;

	for (x = 0; x < length; x++)
	{
		uint32_t addr;

		addr = bitswap<24> (x,23,22,21,0, 20,
							19,18,17,16,
							15,14,13,12,
							11,10,9, 8,
							7, 6, 5, 4,
							3, 2, 1 );

		buf1[addr] = rom[x];
	}

	memcpy(rom, &buf1[0], length);
}

READ32_MEMBER(backfire_state::backfire_speedup_r)
{
	//osd_printf_debug( "%08x\n",m_maincpu->pc());

	if (m_maincpu->pc() == 0xce44) m_maincpu->spin_until_time(attotime::from_usec(400)); // backfire
	if (m_maincpu->pc() == 0xcee4) m_maincpu->spin_until_time(attotime::from_usec(400)); // backfirea

	return m_mainram[0x18/4];
}


void backfire_state::init_backfire()
{
	deco56_decrypt_gfx(machine(), "gfx1"); /* 141 */
	deco56_decrypt_gfx(machine(), "gfx2"); /* 141 */
	deco156_decrypt(machine());
	m_maincpu->set_clock_scale(4.0f); /* core timings aren't accurate */
	descramble_sound();
	m_maincpu->space(AS_PROGRAM).install_read_handler(0x0170018, 0x017001b, read32_delegate(FUNC(backfire_state::backfire_speedup_r), this));
}

GAME( 1995, backfire,  0,        backfire,   backfire, backfire_state, init_backfire, ROT0, "Data East Corporation", "Backfire! (Japan, set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1995, backfirea, backfire, backfire,   backfire, backfire_state, init_backfire, ROT0, "Data East Corporation", "Backfire! (Japan, set 2)", MACHINE_SUPPORTS_SAVE ) // defaults to wheel controls, must change to joystick to play
