// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <asm/io.h>

static int _probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	void *va;
	unsigned int tmp;
	
	va = ioremap(0xE8000000, PAGE_SIZE);
	if (va) {
		tmp = readl(va);
		dev_info(dev, "0xE8000000=0x%08x\n", tmp);
		tmp &= ~(0xf << 4);
		tmp |= 0x1 << 4;
		writel(tmp, va);
		
		tmp = readl(va + 0x8);
		dev_info(dev, "0xE8000008=0x%08x\n", tmp);
		tmp = 0x5 << 16;
		writel(tmp, va + 0x8);
		
		iounmap(va);
	} else {
		dev_err(dev, "Failed to ioremap 0xE8000000\n");
	}
	
	va = ioremap(0xe0107008, PAGE_SIZE);
	if (va) {
		writel(0x2, va);
		
		iounmap(va);
	} else {
		dev_err(dev, "Failed to ioremap 0xe0107008\n");
	}
	
	dev_info(dev, "probe done\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id match_[] = {
	{ .compatible = "210,misc", },
	{ }
};
MODULE_DEVICE_TABLE(of, match_);
#endif

static struct platform_driver driver_ = {
	.driver = {
		.name = "210_misc",
		.of_match_table = of_match_ptr(match_),
	},
	.probe = _probe,
};

static int __init _driver_init(void)
{
	return platform_driver_register(&driver_);
}
fs_initcall(_driver_init);

static void __exit _driver_exit(void)
{
	platform_driver_unregister(&driver_);
}
module_exit(_driver_exit);

MODULE_AUTHOR("gq213 <gaoqiang1211@gmail.com>");
MODULE_LICENSE("GPL v2");
