# Smart GPIO Event & Control Framework (OpenWrt / MT7981)

## Overview
This project implements a **Device Tree–driven GPIO platform driver** for
OpenWrt running on **MediaTek MT7981 (ARM64)** platforms.

The driver supports:
- GPIO output control
- GPIO interrupt (IRQ) handling
- Event notification using `poll()`
- Character device interface (`/dev/gpio_event`)
- Clean BSP integration using **DT overlays**

This project demonstrates **real kernel + BSP development**, not sysfs-based GPIO hacks.

---

## Architecture

