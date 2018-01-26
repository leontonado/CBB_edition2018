#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xd2901226, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xb97a8d7, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x9fd7f8ea, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0x5125e680, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xedbfc33b, __VMLINUX_SYMBOL_STR(__dynamic_dev_dbg) },
	{ 0x74c756f2, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x6da45f2a, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0x2dadad0c, __VMLINUX_SYMBOL_STR(__uio_register_device) },
	{ 0x33ea1df0, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x66ed631d, __VMLINUX_SYMBOL_STR(pci_enable_msix) },
	{ 0xa11b55b2, __VMLINUX_SYMBOL_STR(xen_start_info) },
	{ 0x731dba7a, __VMLINUX_SYMBOL_STR(xen_domain_type) },
	{ 0xe08b3973, __VMLINUX_SYMBOL_STR(dma_supported) },
	{ 0x42c8de35, __VMLINUX_SYMBOL_STR(ioremap_nocache) },
	{ 0x40c450dc, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0x6075bbd, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x51381898, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0x910e0dc, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x84f43e8d, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xc8a3ca91, __VMLINUX_SYMBOL_STR(pci_check_and_mask_intx) },
	{ 0xa6de1c66, __VMLINUX_SYMBOL_STR(pci_intx) },
	{ 0xd21d107b, __VMLINUX_SYMBOL_STR(pci_cfg_access_unlock) },
	{ 0xd41a5045, __VMLINUX_SYMBOL_STR(pci_cfg_access_lock) },
	{ 0x7876d70, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0x5944d015, __VMLINUX_SYMBOL_STR(__cachemode2pte_tbl) },
	{ 0xa50a80c2, __VMLINUX_SYMBOL_STR(boot_cpu_data) },
	{ 0xd2d61936, __VMLINUX_SYMBOL_STR(pci_disable_msix) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x7356cebf, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0xc7f41e24, __VMLINUX_SYMBOL_STR(uio_unregister_device) },
	{ 0x8448f098, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x53e1ca45, __VMLINUX_SYMBOL_STR(pci_bus_type) },
	{ 0x7429842b, __VMLINUX_SYMBOL_STR(pci_enable_sriov) },
	{ 0x696600a5, __VMLINUX_SYMBOL_STR(pci_num_vf) },
	{ 0x227d2f05, __VMLINUX_SYMBOL_STR(pci_disable_sriov) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=uio";


MODULE_INFO(srcversion, "96687AB3B88A138823B5C1B");
