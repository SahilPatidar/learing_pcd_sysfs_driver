#include "pcd_platform_driver_dt_sysfs.h"

int check_permission(int dev_perm,int acc_mode)
{
        if(dev_perm == RDWR)
               return 0;
        if((dev_perm == RDONLY) && (( acc_mode & FMODE_READ ) && !( acc_mode & FMODE_WRITE )))
                return 0;
        if((dev_perm == WRONLY) && (!( acc_mode & FMODE_READ ) && ( acc_mode & FMODE_WRITE )))
                return 0;

        return -EPERM;

}


loff_t pcd_lseek(struct file *filp,loff_t  offset , int whence)
{

	return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data* pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	pr_info("read requested for %zu bytes\n",count);
	pr_info("current file position = %lld\n",*f_pos);

	if((*f_pos + count) > max_size){
	       count = max_size - *f_pos;
	}

	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count)){
	         return -EFAULT;
	}

	*f_pos += count;

	     pr_info("Number of bytes successfully read = %zu\n",count);
        pr_info("Updated file position = %lld\n",*f_pos);

	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t  *f_pos)
{	 struct pcdev_private_data* pcdev_data = (struct pcdev_private_data*)filp->private_data;

        int max_size = pcdev_data->pdata.size;

        pr_info("Write requested for %zu bytes\n",count);
        pr_info("Current file position = %lld\n",*f_pos);

        /* Adjust the 'count' */
        if((*f_pos + count) > max_size)
                count = max_size - *f_pos;

        if(!count){
                pr_err("No space left on the device \n");
                return -ENOMEM;
        }

        /*copy from user */
        if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count)){
                return -EFAULT;
        }
	*f_pos += count;

        pr_info("Number of bytes successfully written = %zu\n",count);
        pr_info("Updated file position = %lld\n",*f_pos);

        /*Return number of bytes which have been successfully written */
        return count;

}



int pcd_open(struct inode* inode,struct file* filp)
{
	int ret;
	int minor_n;
	struct pcdev_private_data *pcdev_data;
	
	minor_n = MINOR(inode->i_rdev);
	
	pr_info("minor number = %d\n",minor_n);
        
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);
	
	filp->private_data = pcdev_data;
	
	ret = check_permission(pcdev_data->pdata.perm,filp->f_mode);
	
	(!ret)?pr_info("open was successful\n"):pr_info("open was unsuccessfull ");


        return ret;
}

int pcd_release(struct inode* inode,struct file* filp)
{
  return 0;
}

