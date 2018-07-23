/* net/dsa/mv88e6071.c - Marvell 88e6071 switch chip support
 * Copyright (c) 2008-2009 Marvell Semiconductor
 * Copyright (c) 2014 Claudio Leite <leitec@staticky.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <net/dsa.h>
#include "mv88e6071.h"

static int reg_read(struct mii_bus *bus, int addr, int reg)
{
	if (bus == NULL)
		return -EINVAL;

	return mdiobus_read_nested(bus, addr, reg);
}

#define REG_READ(addr, reg)					\
	({							\
		int __ret;					\
								\
		__ret = reg_read(mii_bus, addr, reg);		\
		if (__ret < 0)					\
			return __ret;				\
		__ret;						\
	})


static int reg_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	if (bus == NULL)
		return -EINVAL;

	return mdiobus_write_nested(bus, addr, reg, val);
}

#define REG_WRITE(addr, reg, val)				\
	({							\
		int __ret;					\
								\
		__ret = reg_write(mii_bus, addr, reg, val);		\
		if (__ret < 0)					\
			return __ret;				\
	})

static int mv88e6071_phy_read(struct mii_bus *mii_bus, int addr,
                                        int regnum)
{
        int ret;

        ret = reg_write(mii_bus, REG_GLOBAL2, GLOBAL2_SMI_OP,
                                   GLOBAL2_SMI_OP_22_READ | (addr << 5) |
                                   regnum);
        if (ret < 0)
                return ret;

        //ret = _mv88e6xxx_phy_wait(ds);
        //if (ret < 0)
        //        return ret;
	mdelay(1);
        return reg_read(mii_bus, REG_GLOBAL2, GLOBAL2_SMI_DATA);
}

static int mv88e6071_phy_write(struct mii_bus *mii_bus, int addr,
                                         int regnum, u16 val)
{
        int ret;

        ret = reg_write(mii_bus, REG_GLOBAL2, GLOBAL2_SMI_DATA, val);
        if (ret < 0)
                return ret;

        ret = reg_write(mii_bus, REG_GLOBAL2, GLOBAL2_SMI_OP,
                                   GLOBAL2_SMI_OP_22_WRITE | (addr << 5) |
                                   regnum);

        //return _mv88e6xxx_phy_wait(ds);
	return 0;
}

#if 0
static const struct mv88e6xxx_switch_id mv88e6071_table[] = {
	{ PORT_SWITCH_ID_6071, "Marvell 88E6071" },
};

static char *mv88e6071_probe(struct device *host_dev, int sw_addr)
{
	return mv88e6xxx_lookup_name(host_dev, sw_addr, mv88e6071_table,
				     ARRAY_SIZE(mv88e6071_table));
}

static int mv88e6071_setup_global(struct dsa_switch *ds)
{
	u32 upstream_port = dsa_upstream_port(ds);
	int ret;
	u32 reg;

	ret = mv88e6xxx_setup_global(ds);
	if (ret)
		return ret;

	/* Discard packets with excessive collisions, mask all
	 * interrupt sources, enable PPU.
	 */
	REG_WRITE(REG_GLOBAL, GLOBAL_CONTROL,
		  GLOBAL_CONTROL_PPU_ENABLE | GLOBAL_CONTROL_DISCARD_EXCESS);

	/* Configure the upstream port, and configure the upstream
	 * port as the port to which ingress and egress monitor frames
	 * are to be sent.
	 */
	reg = upstream_port << GLOBAL_MONITOR_CONTROL_INGRESS_SHIFT |
		upstream_port << GLOBAL_MONITOR_CONTROL_EGRESS_SHIFT |
		upstream_port << GLOBAL_MONITOR_CONTROL_ARP_SHIFT |
		upstream_port << GLOBAL_MONITOR_CONTROL_MIRROR_SHIFT;
	REG_WRITE(REG_GLOBAL, GLOBAL_MONITOR_CONTROL, reg);

	/* Disable remote management for now, and set the switch's
	 * DSA device number.
	 */
	REG_WRITE(REG_GLOBAL, GLOBAL_CONTROL_2, ds->index & 0x1f);

	return 0;
}
#endif 

static int mv88e6071_switch_reset(struct mii_bus * mii_bus)
{
#if 0 //FIXME	
	int i;
	int ret;
	unsigned long timeout;

	/* Set all ports to the disabled state. */
	for (i = 0; i < MV88E6071_PORTS; i++) {
		ret = REG_READ(REG_PORT(i), PORT_CONTROL);
		REG_WRITE(REG_PORT(i), PORT_CONTROL,
			  ret & ~PORT_CONTROL_STATE_MASK);
	}

	/* Wait for transmit queues to drain. */
	usleep_range(2000, 4000);

	/* Reset the switch. */
	REG_WRITE(REG_GLOBAL, GLOBAL_ATU_CONTROL,
		  GLOBAL_ATU_CONTROL_SWRESET |
		  GLOBAL_ATU_CONTROL_ATUSIZE_1024 |
		  GLOBAL_ATU_CONTROL_ATE_AGE_5MIN);

	/* Wait up to one second for reset to complete. */
	timeout = jiffies + 1 * HZ;
	while (time_before(jiffies, timeout)) {
		ret = REG_READ(REG_GLOBAL, GLOBAL_STATUS);
		if (ret & GLOBAL_STATUS_INIT_READY)
			break;

		usleep_range(1000, 2000);
	}
	if (time_after(jiffies, timeout))
		return -ETIMEDOUT;
#endif
	return 0;
}

static int mv88e6071_setup_global(struct mii_bus *mii_bus)
{
#if 0	
	/* Disable discarding of frames with excessive collisions,
	 * set the maximum frame size to 1536 bytes, and mask all
	 * interrupt sources.
	 */
	REG_WRITE(REG_GLOBAL, GLOBAL_CONTROL, GLOBAL_CONTROL_MAX_FRAME_1536);

	/* Enable automatic address learning, set the address
	 * database size to 1024 entries, and set the default aging
	 * time to 5 minutes.
	 */
	REG_WRITE(REG_GLOBAL, GLOBAL_ATU_CONTROL,
		  GLOBAL_ATU_CONTROL_ATUSIZE_1024 |
		  GLOBAL_ATU_CONTROL_ATE_AGE_5MIN);
#endif
	return 0;
}

#define PHY_SPECIFIC_CONTROL_REGISTER   0x10
#define MARVELL88E6071_PHY_100M                 (1<<13)
#define MARVELL88E6071_PHY_AUTONEG              (1<<12)
#define MARVELL88E6071_PHY_FULLDUPLEX   (1<<8)
#define MARVELL88E6071_PHY_RESTARTAUTO  (1<<9)
#define MARVELL88E6071_PHY_LOOPBACK             (1<<14)
#define PHY_CONTROL_REGISTER    0
#define MVREGADDR_PORT_CONTROL  0x4

int Switch_Speed_Settings(struct mii_bus * mii_bus,  unsigned int switchPort)
{
        int data = 0;

        data = MARVELL88E6071_PHY_100M | MARVELL88E6071_PHY_FULLDUPLEX |
        MARVELL88E6071_PHY_AUTONEG | MARVELL88E6071_PHY_RESTARTAUTO; // auto-neg
        mv88e6071_phy_write(mii_bus, 0x10+ switchPort ,PHY_CONTROL_REGISTER,data | 0x8000);
}

static int mv88e6071_setup_port(struct mii_bus *mii_bus, int p)
{
	int addr = REG_PORT(p);

#if 0
	/* Do not force flow control, disable Ingress and Egress
	 * Header tagging, disable VLAN tunneling, and set the port
	 * state to Forwarding.  Additionally, if this is the CPU
	 * port, enable Ingress and Egress Trailer tagging mode.
	 */
	REG_WRITE(addr, PORT_CONTROL,
		  dsa_is_cpu_port(ds, p) ?
			PORT_CONTROL_TRAILER |
			PORT_CONTROL_INGRESS_MODE |
			PORT_CONTROL_STATE_FORWARDING :
			PORT_CONTROL_STATE_FORWARDING);

	/* Port based VLAN map: give each port its own address
	 * database, allow the CPU port to talk to each of the 'real'
	 * ports, and allow each of the 'real' ports to only talk to
	 * the CPU port.
	 */
	REG_WRITE(addr, PORT_VLAN_MAP,
		  ((p & 0xf) << PORT_VLAN_MAP_DBNUM_SHIFT) |
		   (dsa_is_cpu_port(ds, p) ?
			ds->phys_port_mask :
			BIT(ds->dst->cpu_port)));
#else
	//REG_WRITE(addr, PORT_CONTROL , PORT_CONTROL_STATE_FORWARDING);
#endif	
#if 0	
	/* Port Association Vector: when learning source addresses
	 * of packets, add the address to the address database using
	 * a port bitmap that has only the bit for this port set and
	 * the other bits clear.
	 */
	REG_WRITE(addr, PORT_ASSOC_VECTOR, BIT(p));
#endif

#if 1
        int data=0;             //, i = switchPort;

        //Reset Switch PHY: PHY Control REG Offset is 0, bit 15 is S/W Reset.
        data = mv88e6071_phy_read(mii_bus, 0x10 + p ,PHY_SPECIFIC_CONTROL_REGISTER);

        data &= ~0x30;    //clear bit[5:4] AutoMDI[X]
        data |= (0x20);   // set enable auto-crossover
        mv88e6071_phy_write(mii_bus, 0x10+p,PHY_SPECIFIC_CONTROL_REGISTER,data);
        //reset PHY
        //if(switchPort < MARVELL88E6071_PHY_NUM)
        {
                // Spped Settings 10M/100M/Loopback
                Switch_Speed_Settings(mii_bus, p);
                //if(Switch_Reset_PHY(switchPort))
                //      return -1;
        }
        // Enable Ports State to Forwarding.
        data = reg_read(mii_bus, 0x18+p,MVREGADDR_PORT_CONTROL);
        if((data & 0x03) != 3)
        {
                data |= 0x03;    // set to forwarding state
                reg_write(mii_bus, 0x18+p, MVREGADDR_PORT_CONTROL,data);
        }
#if 1
        if(p == 5 || p == 6) //FIXME , why need force link
        {
                data |= (0x3 << 4);//force link
                reg_write(mii_bus, p+0x18,0x1,data);
        }
#endif
#endif
	return 0;
}
#define MVREGADDR_PORT_BASED_VLAN_MAP   0x6
int marvell88e6071_port_vlan_set(struct mii_bus* mii_bus, unsigned int switchPort, unsigned int vlanMap , unsigned int DataBase)
{

#define PHY_ADDR_88E6071 0x10
#define MII_OP_WRITE    1
#define MII_OP_READ     0
/*
        int switchCtrlMac = 0;
        int phyAddr;
        int switchPortId[MARVELL88E6071_PORT_NUM] = {0, 1, 2, 3, 4, 5};
        */
//        if(switchPort>=MARVELL88E6071_PORT_NUM)
//        {
//                printf("invalid switch port number:%d\r\n", switchPort);
//                return -1;
//        }
//      phyAddr = PHY_ADDR_88E6071 + switchPortId[switchPort] + 8;

        //clear self port bit
        vlanMap&= ~(0x1<<switchPort);

        //use same default MAC-mapping database
        vlanMap&= ~(0xf<<12);
        vlanMap|= (DataBase &0xf) << 12;
//      net_phy_access(switchCtrlMac, MII_OP_WRITE, phyAddr, 6, vlanMap);//Write port based VLAN map register(offset 0x6)
        reg_write(mii_bus,0x18+switchPort, MVREGADDR_PORT_BASED_VLAN_MAP, vlanMap);
        return 0;
}

static int mv88e6071_setup(struct mii_bus *mii_bus)
{
	int i;
	int ret;
	
	ret = mv88e6071_switch_reset(mii_bus);
	if (ret < 0)
		return ret;

	/* @@@ initialise atu */
	ret = mv88e6071_setup_global(mii_bus);
	if (ret < 0)
		return ret;

	for (i = 0; i < MV88E6071_PORTS; i++) {
		ret = mv88e6071_setup_port(mii_bus, i);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int mv88e6071_init(struct mii_bus * mii_bus)
{
	mv88e6071_setup(mii_bus);

	marvell88e6071_port_vlan_set(mii_bus,1 , (1<<5) , 0);
	marvell88e6071_port_vlan_set(mii_bus,5 , (1<<1) , 0);

	marvell88e6071_port_vlan_set(mii_bus,0 , (1<<2)|(1<<6) , 1);
	marvell88e6071_port_vlan_set(mii_bus,2 , (1<<0)|(1<<6) , 1);
	marvell88e6071_port_vlan_set(mii_bus,6 , (1<<0)|(1<<2) , 1);
	return 0;
}
