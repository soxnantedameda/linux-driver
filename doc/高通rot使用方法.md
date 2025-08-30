# 高通rot使用方法指南

## 前言

​    高通qcs610存在一个离线的rot设备，可以对视频buff进行旋转并支持水平反转和垂直翻转，该硬件已对接到v4l2驱动，可以通过v4l2的公共用户API进行调用，该文档对调用流程进行一个分析梳理。

​	该设备支持多用户调用，允许至多16个用户同时使用rot。

​	性能，对于1920x1080NV12数据，一帧的耗时为5ms左右。

## 设备节点

```bash
/dev/video0
```

## 使用流程

### 1.打开视频文件设备

```c
//阻塞模式
fd = open("/dev/video0", O_RDWR);
```

### 2.查询属性、功能

```c
//查询设备的属性
ioctl(int fd, VIDIOC_QUERYCAP, struct v4l2_capability *cap);  

/**
  * struct v4l2_capability - Describes V4L2 device caps returned by VIDIOC_QUERYCAP
  *
  * @driver:	   name of the driver module (e.g. "bttv")
  * @card:	   name of the card (e.g. "Hauppauge WinTV")
  * @bus_info:	   name of the bus (e.g. "PCI:" + pci_name(pci_dev) )
  * @version:	   KERNEL_VERSION
  * @capabilities: capabilities of the physical device as a whole
  * @device_caps:  capabilities accessed via this particular device (node)
  * @reserved:	   reserved fields for future extensions
  */
struct v4l2_capability {
	__u8	driver[16];		/* 驱动名字 */
	__u8	card[32];		/* 设备名字 */	
	__u8	bus_info[32];	/* 总线名字 */
	__u32   version;		/* 版本信息 */	
	__u32	capabilities;	/* 功能 */
	__u32	device_caps;	/* 通过特定设备(节点)访问的能力 */
	__u32	reserved[3];	/* 保留 */	
};

```

示例：

```c
static void _rot_get_fmt_support(int fd)
{
	struct v4l2_fmtdesc fmtdesc;
	int i = 0;
	int ret  = 0;

	memset(&fmtdesc, 0, sizeof(struct v4l2_fmtdesc));
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //或V4L2_BUF_TYPE_VIDEO_OUTPUT
	while (1) {
		unsigned char *p = NULL;

		fmtdesc.index = i;
		ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (ret < 0) {
			break;
		}
		p = (unsigned char *)&fmtdesc.pixelformat;
		printf("index = %d, discription = %s, fmt = %s\n",
			fmtdesc.index, fmtdesc.description, p);
		i++;
	}
}
```

### 3.枚举&设置设备参数

- 在设置格式之前，要先枚举出所有的格式，看一看是否支持要设置的格式，然后再进一步设置 。

```c
//枚举出支持的所有像素格式：VIDIOC_ENUM_FMT
ioctl(int fd, VIDIOC_ENUM_FMT, struct v4l2_fmtdesc *fmtdesc);
//枚举所支持的所有视频采集分辨率:VIDIOC_ENUM_FRAMESIZES
ioctl(int fd, VIDIOC_ENUM_FRAMESIZES, struct v4l2_frmsizeenum *frmsize);
```

- 设置图像格式需要用到**struct v4l2_format**结构体，该结构体描述每帧图像的具体格式，包括帧类型以及图像的长、宽等信息。

```c
//获取设备支持的格式
ioctl(int fd, VIDIOC_G_FMT, struct v4l2_format *fmt);

/**
 * struct v4l2_format - stream data format
 * @type:	enum v4l2_buf_type; type of the data stream
 * @pix:	definition of an image format
 * @pix_mp:	definition of a multiplanar image format
 * @win:	definition of an overlaid image
 * @vbi:	raw VBI capture or output parameters
 * @sliced:	sliced VBI capture or output parameters
 * @raw_data:	placeholder for future extensions and custom formats
 */
struct v4l2_format {
	__u32	 type; 			// 帧类型，应用程序设置
	union {
		struct v4l2_pix_format		pix;     /* V4L2_BUF_TYPE_VIDEO_CAPTURE 视频设备使用*/
		struct v4l2_pix_format_mplane	pix_mp;  /* V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
		struct v4l2_window		win;     /* V4L2_BUF_TYPE_VIDEO_OVERLAY */
		struct v4l2_vbi_format		vbi;     /* V4L2_BUF_TYPE_VBI_CAPTURE */
		struct v4l2_sliced_vbi_format	sliced;  /* V4L2_BUF_TYPE_SLICED_VBI_CAPTURE */
		struct v4l2_sdr_format		sdr;     /* V4L2_BUF_TYPE_SDR_CAPTURE */
		struct v4l2_meta_format		meta;    /* V4L2_BUF_TYPE_META_CAPTURE */
		__u8	raw_data[200];                   /* user-defined */
	} fmt;
};
```

- 输入buff的类型设置成V4L2_BUF_TYPE_VIDEO_OUTPUT，输出buff的类型设置成V4L2_BUF_TYPE_VIDEO_CAPTURE
- 当旋转90或270度时，输出buff应交换宽高

```c
static int _rot_set_src_format(int fd, unsigned int wight, unsigned int height, unsigned int format)
{
	struct v4l2_format fmt_out;
	int ret = 0;

	memset(&fmt_out, 0, sizeof(struct v4l2_format));
	fmt_out.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt_out.fmt.pix.width = wight;
	fmt_out.fmt.pix.height = height;
	fmt_out.fmt.pix.pixelformat = format;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt_out);
	if (ret < 0) {
		printf("set src format failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int _rot_set_dst_format(int fd, unsigned int wight, unsigned int height, unsigned int format)
{
	struct v4l2_format fmt_out;
	int ret = 0;

	//printf("%s %d\n", __func__, __LINE__);
	memset(&fmt_out, 0, sizeof(struct v4l2_format));
	fmt_out.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt_out.fmt.pix.width = wight;
	fmt_out.fmt.pix.height = height;
	fmt_out.fmt.pix.pixelformat = format;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt_out);
	if (ret < 0) {
		printf("set dst format failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
```



- 设置帧率，需要传入struct v4l2_streamparm结构体

```c
ioctl(int fd, VIDIOC_S_PARM, struct v4l2_streamparm *streamparm);
```

示例

```c
static int _rot_set_frame_rate(int fd, int timeperframe)
{
	struct v4l2_streamparm parm;
	int ret = 0;

	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	parm.parm.output.timeperframe.numerator = 1;
	parm.parm.output.timeperframe.denominator = timeperframe;
	ret = ioctl(fd, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		printf("set parm failed, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
```

### 4.申请帧缓存

- 读取数据的方式有两种，一种是 **read** 方式（对应设备功能返回的 **V4L2_CAP_READWRITE** ）；另一种则是 **streaming** 方式 (使用mmap，对应设备功能返回的 **V4L2_CAP_STREAMING** )

```c
//申请帧缓存
ioctl(int fd, VIDIOC_REQBUFS, struct v4l2_requestbuffers *reqbuf);

struct v4l2_requestbuffers {
	__u32			count;		/* 帧缓存区的数量 */
	__u32			type;		/* enum v4l2_buf_type  缓冲帧数据格式*/
	__u32			memory;		/* enum v4l2_memory 是内存映射还是用户指针方式*/
	__u32			capabilities;
	__u32			reserved[1];
    
};
```

注意：需要申请两个缓存队列，一个为输入源(source)队列，一个输出源(destination)队列，再高通平台上常用gbm内存(本质是ion)，就使用V4L2_MEMORY_USERPTR。

例子：

```c


static int _rot_request_buf(int fd, enum v4l2_memory m_type)
{
	struct v4l2_requestbuffers buff_out;
	struct v4l2_requestbuffers buff_cap;
	int ret = 0;

	memset(&buff_out, 0, sizeof(struct v4l2_requestbuffers));
	buff_out.count = BUFCOUNT;
	buff_out.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buff_out.memory = m_type;		//V4L2_MEMORY_USERPTR or V4L2_MEMORY_MMAP
	ret = ioctl(fd, VIDIOC_REQBUFS, &buff_out);
	if (ret < 0) {
		printf("rot: request captrue buffer error, ret = %d\n", ret);
		return -ret;
	}

	memset(&buff_cap, 0, sizeof(struct v4l2_requestbuffers));
	buff_cap.count = BUFCOUNT;
	buff_cap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buff_cap.memory = m_type;		//V4L2_MEMORY_USERPTR or V4L2_MEMORY_MMAP
	ret = ioctl(fd, VIDIOC_REQBUFS, &buff_cap);
	if (ret < 0) {
		printf("rot: request output buffer error, ret = %d\n", ret);
		return -ret;
	}

	return 0;
}
```

### 5.设置buff处理方式

支持水平反转和垂直翻转，支持旋转(0,90,180,270)

```c
* @hflip: horizontal flip (1-flip)
* @vflip: vertical flip (1-flip)
* @rotate: rotation angle (0,90,180,270)
     
ioctl(int fd, VIDIOC_REQBUFS, struct  struct v4l2_control *ctrl);
```

示例

```c
static int _rot_set_rotator(int fd, int rot, int hflip, int vflip)
{
	struct v4l2_control ctrl;
	int ret = 0;

	memset(&ctrl, 0, sizeof(struct v4l2_control));
	ctrl.id = V4L2_CID_ROTATE;
	ctrl.value = rot;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set rotator rot failed!, ret = %d\n", ret);
		return ret;
	}

	ctrl.id = V4L2_CID_HFLIP;
	ctrl.value  = !!hflip;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set rotator hflip failed!, ret = %d\n", ret);
		return ret;
	}

	ctrl.id = V4L2_CID_VFLIP;
	ctrl.value  = !!vflip;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0) {
		printf("set rotator vflip failed!, ret = %d\n", ret);
		return ret;
	}

	return 0;
}
```

### 6.开始执行转换

```C
ioctl(int fd, VIDIOC_STREAMON, int *type);
```

示例

```c
static int _rot_stream_on(int fd)
{
	enum v4l2_buf_type type;
	int ret = 0;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON CAPTURE failure");
		return ret;
	}
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON OUTPUT failure");
		return ret;
	}

	return 0;
}
```

### 7.buff入队

- 将申请的缓冲帧(包括输入和输出)依次放入缓冲帧输入队列，等待被图像采集设备依次填满

```c
ioctl(int fd, VIDIOC_DQBUF, struct v4l2_buffer *buf);

/**
 * struct v4l2_buffer - video buffer info
 * @index:	id number of the buffer
 * @type:	enum v4l2_buf_type; buffer type (type == *_MPLANE for
 *		multiplanar buffers);
 * @bytesused:	number of bytes occupied by data in the buffer (payload);
 *		unused (set to 0) for multiplanar buffers
 * @flags:	buffer informational flags
 * @field:	enum v4l2_field; field order of the image in the buffer
 * @timestamp:	frame timestamp
 * @timecode:	frame timecode
 * @sequence:	sequence count of this frame
 * @memory:	enum v4l2_memory; the method, in which the actual video data is
 *		passed
 * @offset:	for non-multiplanar buffers with memory == V4L2_MEMORY_MMAP;
 *		offset from the start of the device memory for this plane,
 *		(or a "cookie" that should be passed to mmap() as offset)
 * @userptr:	for non-multiplanar buffers with memory == V4L2_MEMORY_USERPTR;
 *		a userspace pointer pointing to this buffer
 * @fd:		for non-multiplanar buffers with memory == V4L2_MEMORY_DMABUF;
 *		a userspace file descriptor associated with this buffer
 * @planes:	for multiplanar buffers; userspace pointer to the array of plane
 *		info structs for this buffer
 * @length:	size in bytes of the buffer (NOT its payload) for single-plane
 *		buffers (when type != *_MPLANE); number of elements in the
 *		planes array for multi-plane buffers
 * @request_fd: fd of the request that this buffer should use
 *
 * Contains data exchanged by application and driver using one of the Streaming
 * I/O methods.
 */
struct v4l2_buffer {
	__u32			index;
	__u32			type;
	__u32			bytesused;
	__u32			flags;
	__u32			field;
	struct timeval		timestamp;
	struct v4l2_timecode	timecode;
	__u32			sequence;

	/* memory location */
	__u32			memory;
	union {
		__u32           offset;
		unsigned long   userptr;
		struct v4l2_plane *planes;
		__s32		fd;
	} m;
	__u32			length;
	__u32			reserved2;
	union {
		__s32		request_fd;
		__u32		reserved;
	};
};
```

注意：需要将输入和输出buff需全部入队，一般先入队输出源(destination)，再入队输入源(source)。

示例：

```c
static int _rot_queue_src_buf(int rot_fd, unsigned int index, struct buffer *buf)
{
	struct v4l2_buffer rot_buf;
	int ret = 0;

	memset(&rot_buf, 0, sizeof(struct v4l2_buffer));
	rot_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rot_buf.memory = V4L2_MEMORY_USERPTR;
	rot_buf.index = index;
	rot_buf.m.fd = buf->fd;
	rot_buf.length = buf->length;
	rot_buf.bytesused = buf->length;
	ret = ioctl(rot_fd, VIDIOC_QBUF, &rot_buf);
	if (ret < 0) {
		printf("queue rot cap %d buffer failed, ret = %d\n", index, ret);
		return ret;
	}

	return 0;
}

static int _rot_queue_dst_buf(int rot_fd, unsigned int index, struct buffer *buf)
{
	struct v4l2_buffer rot_buf;
	int ret = 0;

	memset(&rot_buf, 0, sizeof(struct v4l2_buffer));
	rot_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rot_buf.memory = V4L2_MEMORY_USERPTR;
	rot_buf.index = index;
	rot_buf.m.fd = buf->fd;
	rot_buf.length = buf->length;
	rot_buf.bytesused = buf->length;
	ret = ioctl(rot_fd, VIDIOC_QBUF, &rot_buf);
	if (ret < 0) {
		printf("queue rot cap %d buffer failed, ret = %d\n", index, ret);
		return ret;
	}

	return 0;
}
```



### 8.buff出队

```C
ioctl(int fd, VIDIOC_DQBUF, struct v4l2_buffer *buf);
```

示例

```c
struct v4l2_buffer dqbuf_out;
struct v4l2_buffer dqbuf_cap;

static int _rot_dequeue_dst_buf(int fd, unsigned int *index)
{
	struct v4l2_buffer dqbuf;
	int ret = 0;

	memset(&dqbuf, 0, sizeof(struct v4l2_buffer));
	dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_DQBUF, &dqbuf);
	if (ret < 0) {
		printf("rot dequeue cap buffer failed, ret = %d\n", ret);
		return ret;
	}
	*index = dqbuf.index;  //出队的buffer index

	return 0;
}

static int _rot_dequeue_dst_buf(int fd, unsigned int *index)
{
	struct v4l2_buffer dqbuf;
	int ret = 0;

	memset(&dqbuf, 0, sizeof(struct v4l2_buffer));
	dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_DQBUF, &dqbuf);
	if (ret < 0) {
		printf("rot dequeue cap buffer failed, ret = %d\n", ret);
		return ret;
	}
	*index = dqbuf.index;  //出队的buffer index

	return 0;
}
```

注意：出队后的buffer处理完成(如写入到文件)后需重新入队，避免出现队列中没buffer的情况。

### 9.停止转换

```c
enum v4l2_buf_type type 

ioctl(int fd, VIDIOC_STREAMOFF, int *type);
```

示例

```c
static int _rot_stream_off(int fd)
{
	enum v4l2_buf_type type;
	int ret = 0;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMOFF CAPTURE failure");
		return ret;
	}

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMOFF OUTPUT failure");
		return ret;
	}

	return 0;
}
```

