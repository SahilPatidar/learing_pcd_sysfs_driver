#include "pcd_platform_driver_dt_sysfs.h"

struct device_config pcdev_config[] =
{	
	[PCDEVA1X] = {.config_item1 = 60,.config_item2 = 21},
        [PCDEVB1X] = {.config_item1 = 50,.config_item2 = 22},
        [PCDEVC1X] = {.config_item1 = 40,.config_item2 = 23},
        [PCDEVD1X] = {.config_item1 = 30,.config_item2 = 24},

};


struct pcdrv_private_data pcdrv_data;


struct file_operations pcd_fops = {
	.open = pcd_open,
        .release = pcd_release,
        .read = pcd_read,
        .write = pcd_write,
        .llseek = pcd_lseek,
        .owner = THIS_MODULE
};

ssize_t show_serial_num(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);

	return sprintf(buf,"%s\n",dev_data->pdata.serial_number);
}
ssize_t show_max_size(struct device *dev,struct device_attribute *attr,char *buf)
{
	 struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);

        return sprintf(buf,"%d\n",dev_data->pdata.size);
}
ssize_t store_max_size(struct device *dev,struct device_attribute *attr,const char *buf,size_t count)
{
	long result;
	int ret;	
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	ret = kstrtol(buf,10,&result);
	if(ret)
		return ret;
	dev_data->pdata.size = result;
	dev_data->buffer = krealloc(dev_data->buffer,dev_data->pdata.size,GFP_KERNEL);
        return count;
}


static DEVICE_ATTR(max_size,S_IRUGO|S_IWUSR,show_max_size,store_max_size);
static DEVICE_ATTR(serial_num,S_IRUGO,show_serial_num,NULL);

struct attribute *pcd_attrs[] = {

	&dev_attr_max_size.attr,
	&dev_attr_serial_num.attr,
	NULL

};

struct attribute_group pcd_attr_group = 
{
	.attrs = pcd_attrs
};

int pcd_sysfs_create_files(struct device *pcd_dev)
{
	int ret;
#if 0
	ret = sysfs_create_file(&pcd_dev->kobj,&dev_attr_max_size.attr);
	if(ret)
		return ret;

	return sysfs_create_file(&pcd_dev->kobj,&dev_attr_serial_num.attr);
#endif
	return sysfs_create_group(&pcd_dev->kobj,&pcd_attr_group);
}

int  pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);

	cdev_del(&dev_data->cdev);
	
	pcdrv_data.total_devices--;
	dev_info(&pdev->dev,"A device is removed\n");
	return 0;
}

struct pcdev_platform_data* pcdev_get_platdata_from_dt(struct device *dev)
{
	struct device_node *dev_node = dev->of_node;
	struct pcdev_platform_data *pdata;
	if(!dev_node)
		return NULL;
	
	pdata = devm_kzalloc(dev,sizeof(*pdata),GFP_KERNEL);
	if(!pdata){
		dev_info(dev,"Connot allocate memory\n");
		return ERR_PTR(-ENOMEM);
	}
	if(of_property_read_string(dev_node,"org,device-serial-num",&pdata->serial_number) ){
		dev_info(dev,"missing serial number\n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node,"org,size",&pdata->size) ){
                dev_info(dev,"missing serial number property\n");
                return ERR_PTR(-EINVAL);
        }
	if(of_property_read_u32(dev_node,"org,perm",&pdata->perm) ){
                dev_info(dev,"missing size property\n");
                return ERR_PTR(-EINVAL);
        }

	return pdata;
}

struct of_device_id org_pcdev_dt_match[];

int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;

	struct pcdev_private_data *dev_data;
	struct pcdev_platform_data *pdata;
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;
	int driver_data;
	dev_info(dev, "device detected successfull\n");
	
	match = of_match_device(of_match_ptr(org_pcdev_dt_match),dev);

	if(match){
		pdata = pcdev_get_platdata_from_dt(dev);
		if(IS_ERR(pdata))
         	       return PTR_ERR(pdata);
		driver_data = (int)match->data;
	}else{
		pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);
                driver_data = pdev->id_entry->driver_data;
	}
	
	if(!pdata){
	       dev_info(dev,"no platform data available\n");
	       return -EINVAL;
	 
	}
	

	dev_data = devm_kzalloc(dev,sizeof(*dev_data),GFP_KERNEL);
	if(!dev_data){
	    dev_info(dev,"cannot alloc memory\n");
	    return -ENOMEM;
	}
 	
	/*pdev->dev.driver_data = dev_data;  OR */ 
	dev_set_drvdata(dev,dev_data);

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;
	dev_info(dev,"deivce serial number = %s\n",dev_data->pdata.serial_number);
	dev_info(dev,"device size = %d\n",dev_data->pdata.size);
	dev_info(dev,"device permission = %d\n",dev_data->pdata.perm);
	dev_info(dev,"Config item1 = %d\n",pcdev_config[driver_data].config_item1);
	dev_info(dev,"Config item2 = %d\n",pcdev_config[driver_data].config_item2);


	dev_data->buffer = devm_kzalloc(dev,dev_data->pdata.size,GFP_KERNEL);
        if(!dev_data->buffer){
	     dev_info(dev,"buffer mem alloc failed\n");
	     return -ENOMEM;
	     
	}

	dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;
 
	cdev_init(&dev_data->cdev,&pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret<0){
		dev_info(dev,"cdev add failed\n");
		return ret;
	}

	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,dev,dev_data->dev_num,NULL,"pcdev-%d",pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd)){
		dev_err(dev,"device create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
	}
	pcdrv_data.total_devices++;
	
	ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);

	if(ret){
		device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);
		return ret;
	}

	pr_info("A device probe was successsful\n");
	return 0;
}

struct platform_device_id pcdev_ids[] = {
	 {.name = "pcdev-A1x",.driver_data = PCDEVA1X},
	 {.name = "pcdev-B1x",.driver_data = PCDEVB1X},
	 {.name = "pcdev-C1x",.driver_data = PCDEVC1X},
	 {.name = "pcdev-D1x",.driver_data = PCDEVD1X},
	 {}
};

struct of_device_id org_pcdev_dt_match[] = 
{
	{.compatible = "pcdev-A1x",.data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x",.data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x",.data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x",.data = (void*)PCDEVD1X},
	{}
};

struct platform_driver pcd_platform_driver = {
         .probe = pcd_platform_driver_probe,
	 .remove = pcd_platform_driver_remove,
	 .id_table = pcdev_ids,
	 .driver = {
	 	.name = "pseudo-char-device",
		.of_match_table =of_match_ptr(org_pcdev_dt_match)
			
	 }
};

#define MAX_DEVICES 10
static int __init pcd_platform_driver_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");

	if(ret < 0){
		pr_err("Alloc chardev failed\n");
	  	return ret;
	}	
	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd)){
		pr_err("class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
	        unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
		return ret;
	}
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver loaded\n");
	return 0;
}

static void __exit pcd_platform_driver_cleanup(void)
{
	platform_driver_unregister(&pcd_platform_driver);
 	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
	pr_info("pcd platform driver unloaded\n");
}


module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SP");
MODULE_DESCRIPTION("pcd platform driver");
