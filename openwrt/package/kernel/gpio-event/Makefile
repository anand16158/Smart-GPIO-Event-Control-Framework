include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpio-event
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/gpio-event
  SUBMENU:=Other modules
  TITLE:=Smart GPIO Event Driver
  FILES:=$(PKG_BUILD_DIR)/gpio_event.ko
  AUTOLOAD:=$(call AutoLoad,60,gpio_event)
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,gpio-event))
