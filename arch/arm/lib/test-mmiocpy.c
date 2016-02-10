#include <linux/io.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>

extern void mmiocpy(void *, const void *, size_t);

struct page *buf;

static u32 set;
static const int order = 1;
static u16 len, src, dst;
static struct debugfs_blob_wrapper dat;
static struct dentry *ddir, *dlen, *dsrc, *ddst, *dset, *ddat, *dcpy;

ssize_t mcpy(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	char c;

	if (src > dat.size || dst > dat.size)
		return -EIO;
	if (src + len > dat.size || dst + len > dat.size)
		return -EIO;

	if (count)
		if (get_user(c, buf))
			return -EFAULT;

	if (set) {
		if (count)
			memset(dat.data, c, dat.size);
		else
			memcpy(dat.data, dat.data + dat.size, dat.size);
	}

	mmiocpy(dat.data + dst, dat.data + dat.size + src, len);

	return count;
}

struct file_operations cpy = { .write = mcpy };

int debugfs_init(void)
{
	int i, d;
	char *p;

	buf = alloc_pages(GFP_KERNEL, order + 1);
	if (!buf)
		return -ENOMEM;
	dat.data = page_address(buf);
	dat.size = PAGE_SIZE << order;
	for (i=0, d=0, p=dat.data; i < dat.size; i++, d %= 255)
		p[dat.size + i] = p[i] = ++d;
	ddir = debugfs_create_dir("mmiocpy", NULL);
	dlen = debugfs_create_u16("len", 0664, ddir, &len);
	dsrc = debugfs_create_u16("src", 0664, ddir, &src);
	ddst = debugfs_create_u16("dst", 0664, ddir, &dst);
	ddat = debugfs_create_blob("dat", 0664, ddir, &dat);
	dset = debugfs_create_bool("set", 0664, ddir, &set);
	dcpy = debugfs_create_file("cpy", 0200, ddir, NULL, &cpy);

	return 0;
}

void debugfs_exit(void)
{
	debugfs_remove_recursive(ddir);
	if (buf)
		__free_pages(buf, order + 1);
}

module_init(debugfs_init);
module_exit(debugfs_exit);
