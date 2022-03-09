#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9f593a9b, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x4302d0eb, "free_pages" },
	{ 0xbfbf4434, "__register_chrdev" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0xed5f6e, "kmem_cache_alloc_trace" },
	{ 0x25931311, "kmalloc_caches" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x977f511b, "__mutex_init" },
	{ 0x6a5cb5ee, "__get_free_pages" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x800473f, "__cond_resched" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x37110088, "remove_wait_queue" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x4afb2238, "add_wait_queue" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xaad8c7d6, "default_wake_function" },
	{ 0xf0ca6d41, "current_task" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xb5136dc7, "mutex_lock_interruptible" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xf21017d9, "mutex_trylock" },
	{ 0x56470118, "__warn_printk" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "20CB4E14D74DAD7ACD21BC5");
