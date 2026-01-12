# Architecture Notes

- GPIO ownership handled exclusively in kernel
- IRQ-based event handling
- No sysfs GPIO usage
- DT overlay allows feature-based enablement
- Suitable for routers, APs, embedded gateways
