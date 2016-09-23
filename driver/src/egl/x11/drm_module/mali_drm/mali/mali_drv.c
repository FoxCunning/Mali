/*
 * Copyright (C) 2010, 2012-2013, 2015 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/component.h>
#include <linux/vermagic.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <drm/drm_of.h>

#include <drm/drmP.h>
#include <drm/drm_mem_util.h>

#include "../include/mali_drm.h"
#include "mali_drv.h"


static struct platform_device *pdev;

static char mali_drm_device_name[] = "mali_drm";

#if 0
static const struct drm_device_id dock_device_ids[] =
{
	{"MALIDRM", 0},
	{"", 0},
};
#endif

static int mali_driver_load(struct drm_device *dev, unsigned long chipset)
{
	drm_mali_private_t *dev_priv;
	printk(KERN_ERR "DRM: mali_driver_load start\n");

	//dev_priv = drm_calloc_large(1, sizeof(drm_mali_private_t));//, DRM_MEM_DRIVER);
	dev_priv = kzalloc(sizeof(drm_mali_private_t), GFP_KERNEL);

	if (dev_priv == NULL)
	{
		return -ENOMEM;
	}

	idr_init(&dev_priv->object_idr);

	dev->dev_private = (void *)dev_priv;

	if (NULL == dev->platformdev)
	{
		dev->platformdev = platform_device_register_simple(mali_drm_device_name, 0, NULL, 0);
		pdev = dev->platformdev;
	}

	dev_priv->chipset = chipset;

	printk(KERN_ERR "DRM: mali_driver_load done\n");

	return 0;
}

static int mali_driver_unload(struct drm_device *dev)
{
	drm_mali_private_t *dev_priv = dev->dev_private;
	printk(KERN_ERR "DRM: mali_driver_unload start\n");

	idr_destroy(&dev_priv->object_idr);
	kfree(dev_priv);

	printk(KERN_ERR "DRM: mali_driver_unload done\n");

	return 0;
}

static const struct file_operations mali_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = drm_legacy_mmap,
	.poll = drm_poll,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

static int mali_driver_open(struct drm_device *dev, struct drm_file *file)
{
	mali_file_private *file_priv;

	DRM_DEBUG_DRIVER("\n");
	file_priv = kmalloc(sizeof(*file_priv), GFP_KERNEL);
	if (!file_priv)
		return -ENOMEM;

	file->driver_priv = file_priv;

	INIT_LIST_HEAD(&file_priv->obj_list);

	return 0;
}

static void mali_postclose(struct drm_device *dev, struct drm_file *file)
{
	mali_file_private *file_priv = file->driver_priv;

	kfree(file_priv);
}

static struct drm_driver driver =
{
	.driver_features = DRIVER_PRIME,//DRIVER_USE_PLATFORM_DEVICE,
	.load = mali_driver_load,
	.unload = mali_driver_unload,
	.open = mali_driver_open,
	.preclose = mali_reclaim_buffers_locked,
	.lastclose = mali_lastclose,
	.postclose = mali_postclose,
	.context_dtor = NULL,
	.dma_quiescent = mali_idle,
	.ioctls = mali_ioctls,
	
	.fops = &mali_driver_fops,
	
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};


static int compare_of(struct device *dev, void *data)
{
	return dev->of_node == data;
}

static int mali_bind(struct device *dev)
{
	return drm_platform_init(&driver, to_platform_device(dev));
}

static void mali_unbind(struct device *dev)
{
	drm_put_dev(dev_get_drvdata(dev));
}

static const struct component_master_ops mali_master_ops = {
	.bind = mali_bind,
	.unbind = mali_unbind,
};

static int mali_platform_drm_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
#if 0
	struct device_node *node = dev->of_node;
	struct device_node *child_np;
	struct component_match *match = NULL;

	printk(KERN_ERR "DRM: mali_platform_drm_probe start\n");
	

	dma_set_coherent_mask(dev, DMA_BIT_MASK(32));

	of_platform_populate(node, NULL, NULL, dev);

	child_np = of_get_next_available_child(node, NULL);

	while (child_np) {
		component_match_add(dev, &match, compare_of, child_np);
		of_node_put(child_np);
		child_np = of_get_next_available_child(node, child_np);
	}

	ret = component_master_add_with_match(dev, &mali_master_ops, match);
#else
	printk(KERN_ERR "DRM: mali_platform_drm_probe start\n");

	dev_err(dev, "Probing: %s\n", dev->of_node->name);

	ret = drm_of_component_probe(dev, compare_of, &mali_master_ops);

	if (!ret)
		ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
#endif

	printk(KERN_ERR "DRM: mali_platform_drm_probe done\n");
	return ret;
}

static int mali_platform_drm_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &mali_master_ops);
	of_platform_depopulate(&pdev->dev);

	return 0;
}

static int mali_platform_drm_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int mali_platform_drm_resume(struct platform_device *dev)
{
	return 0;
}

static const struct of_device_id mali_dt_ids[] = {
	{ .compatible = "arm,mali-400", },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, mali_dt_ids);

static struct platform_driver platform_drm_driver =
{
	.probe = mali_platform_drm_probe,
	.remove = mali_platform_drm_remove,
	.suspend = mali_platform_drm_suspend,
	.resume = mali_platform_drm_resume,
	.driver = {
		.name = mali_drm_device_name,
		.owner = THIS_MODULE,
		.of_match_table = mali_dt_ids,
	},
};


static int __init mali_init(void)
{
	printk(KERN_ERR "DRM: mali_init\n");
	driver.num_ioctls = mali_max_ioctl;
	return platform_driver_register(&platform_drm_driver);
}

static void __exit mali_exit(void)
{
	printk(KERN_ERR "DRM: mali_exit\n");
	platform_driver_unregister(&platform_drm_driver);
}

module_init(mali_init);
module_exit(mali_exit);

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_AUTHOR("ARM Ltd.");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");
