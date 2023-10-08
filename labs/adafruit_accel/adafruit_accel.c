#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>

/* LSM303DLHC register map */
#define CTRL_REG1_A 0x20
#define CTRL_REG2_A 0x21
#define CTRL_REG3_A 0x22
#define CTRL_REG4_A 0x23
#define CTRL_REG5_A 0x24
#define CTRL_REG6_A 0x25
#define REFERENCE_A 0x26
#define STATUS_REG_A 0x27
#define OUT_X_L_A 0x28
#define OUT_X_H_A 0x29
#define OUT_Y_L_A 0x2A
#define OUT_Y_H_A 0x2B
#define OUT_Z_L_A 0x2C
#define OUT_Z_H_A 0x2D
#define FIFO_CTRL_REG_A 0x2E
#define FIFO_SRC_REG_A 0x2F
#define INT1_CFG_A 0x30
#define INT1_SOURCE_A 0x31
#define INT1_THS_A 0x32
#define INT1_DURATION_A 0x33
#define INT2_CFG_A 0x34
#define INT2_SOURCE_A 0x35
#define INT2_THS_A 0x36
#define INT2_DURATION_A 0x37
#define CLICK_CFG_A 0x38
#define CLICK_SRC_A 0x39
#define CLICK_THS_A 0x3A
#define TIME_LIMIT_A 0x3B
#define TIME_LATENCY_A 0x3C
#define TIME_WINDOW_A 0x3D

enum lsm303dlhc_accel_axis {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

/* 
 * we will get the gpio using an index added to gpio
 * name in the device tree. We can get also without
 * this index
 */
#define LSM303DLHC_GPIO_NAME "int"

/* CLICK_CFG_A: click config masks */
#define SINGLE_CLICK_X_EN (1 << 0)
#define SINGLE_CLICK_Y_EN (1 << 2)
#define SINGLE_CLICK_Z_EN (1 << 4)

/* CTRL_REG1_A: power mode and ODR setting */
#define RATE(x) ((x) << 4)

/* CTRL_REG3_A: INT1 config mask */
#define I1_CLICK (1 << 8)

/* CTRL_REG4_A: resolution and full scale select */
#define HI_RESOLUTION (1 << 3)
#define FS_2G (0b11 << 4)

/* CTRL_REG5_A: FIFO enable */
#define FIFO_EN (1 << 7)

/* FIFO_CTRL_REG_A: FIFO mode settings */
#define FIFO_MODE(x) (((x) & 0x3) << 6)
#define FIFO_MODE_BYPASS 0b00
#define FIFO_MODE_FIFO 0b01
#define FIFO_MODE_STREAM 0b11

#define ACCEL_SCALE_FACTOR_MICRO 0 /* TODO: comptue scaling factor */

struct axis_triple {
	int x;
	int y;
	int z;
};

struct lsm303dlhc_data {
	struct gpio_desc *gpio;
	struct regmap *regmap;
	struct iio_trigger *trig;
	struct device *dev;
	struct axis_triple saved;
	u8 data_range;
	u8 click_threshold;
	u8 click_duration;
	u8 click_axis_control;
	u8 data_rate;
	u8 fifo_mode;
	int irq;
	int ev_enable;
	u32 int_mask;
	s64 timestamp;
};

/* set the events */
static const struct iio_event_spec lsm303dlhc_event = {
	.type = IIO_EV_TYPE_THRESH,
	.dir = IIO_EV_DIR_EITHER,
	.mask_separate = BIT(IIO_EV_INFO_VALUE) | BIT(IIO_EV_INFO_PERIOD)
};

#define LSM303DLHC_CHANNEL(reg, axis, idx) \
	{                                  \
		.type = IIO_ACCEL, \
	.modified = 1, \
	.channel2 = IIO_MOD_##axis,	\
	.address = reg, \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) | \
	BIT(IIO_CHAN_INFO_SAMP_FREQ), \
	.scan_index = idx, \
	.scan_type = { \
			.sign = 's', \
			.realbits = 16, \
			.storagebits = 16, \
			.endianness = IIO_LE, \
		}, \
	.event_spec = &lsm303dlhc_event, \
	.num_event_specs = 1       \
	}

static const struct iio_chan_spec lsm303dlhc_channels[] = {
	LSM303DLHC_CHANNEL(OUT_X_L_A, X, 0),
	LSM303DLHC_CHANNEL(OUT_Y_L_A, Y, 1),
	LSM303DLHC_CHANNEL(OUT_Z_L_A, Z, 2),
	IIO_CHAN_SOFT_TIMESTAMP(3),
};

static int lsm303dlhc_read_raw(struct iio_dev *indio_dev,
			       struct iio_chan_spec const *chan, int *val,
			       int *val2, long mask)
{
	struct lsm303dlhc_data *data = iio_priv(indio_dev);
	__le16 regval;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		/*
		 * Data is stored in adjacent registers:
		 * OUT_{X,Y,Z}_L_A contain the least significant byte
		 * and OUT_{X,Y,Z}_L_A + 1 the most significant byte
		 * we are reading 2 bytes and storing in a __le16
		 */
		ret = regmap_bulk_read(data->regmap, chan->address, &regval,
				       sizeof(regval));
		if (ret < 0)
			return ret;

		*val = sign_extend32(le16_to_cpu(regval), 16);

		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		*val = 0;
		*val2 = ACCEL_SCALE_FACTOR_MICRO;
		return IIO_VAL_INT_PLUS_MICRO;

	default:
		return -EINVAL;
	}
}

static int lsm303dlhc_write_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int val,
				int val2, long mask)
{
	struct lsm303dlhc_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_SAMP_FREQ:
		data->data_rate = RATE(val);
		return regmap_write(data->regmap, CTRL_REG1_A, data->data_rate);
	default:
		return -EINVAL;
	}
}

static int lsm303dlhc_read_event(struct iio_dev *indio_dev,
				 const struct iio_chan_spec *chan,
				 enum iio_event_type type,
				 enum iio_event_direction dir,
				 enum iio_event_info info, int *val, int *val2)
{
	struct lsm303dlhc_data *data = iio_priv(indio_dev);

	switch (info) {
	case IIO_EV_INFO_VALUE:
		*val = data->click_threshold;
		break;
	case IIO_EV_INFO_PERIOD:
		*val = data->click_duration;
		break;
	default:
		return -EINVAL;
	}

	return IIO_VAL_INT;
}

static int lsm303dlhc_write_event(struct iio_dev *indio_dev,
				  const struct iio_chan_spec *chan,
				  enum iio_event_type type,
				  enum iio_event_direction dir,
				  enum iio_event_info info, int val, int val2)
{
	struct lsm303dlhc_data *data = iio_priv(indio_dev);

	switch (info) {
	case IIO_EV_INFO_VALUE:
		data->click_threshold = val;
		return regmap_write(data->regmap, CLICK_THS_A,
				    data->click_threshold);

	case IIO_EV_INFO_PERIOD:
		data->click_duration = val;
		return regmap_write(data->regmap, TIME_LIMIT_A,
				    data->click_duration);
	default:
		return -EINVAL;
	}
}

static const struct regmap_config lsm303dlhc_i2c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static const struct iio_info lsm303dlhc_info = {
	.read_raw = lsm303dlhc_read_raw,
	.write_raw = lsm303dlhc_write_raw,
	.read_event_value = lsm303dlhc_read_event,
	.write_event_value = lsm303dlhc_write_event,
};

/* Available channels, later enabled from user space or using active_scan_mask */
static const unsigned long lsm303dlhc_accel_scan_masks[] = {
	BIT(AXIS_X) | BIT(AXIS_Y) | BIT(AXIS_Z), 0
};

/* Interrupt service routine */
static irqreturn_t lsm303dlhc_event_handler(int irq, void *handle)
{
	u32 click_stat, int_stat;
	int ret;
	struct iio_dev *indio_dev = handle;
	struct lsm303dlhc_data *data = iio_priv(indio_dev);

	data->timestamp = iio_get_time_ns(indio_dev);
	/*
	 * ACT_TAP_STATUS should be read before clearing the interrupt
	 * Avoid reading ACT_TAP_STATUS in case TAP detection is disabled
	 * Read the ACT_TAP_STATUS if any of the axis has been enabled
	 */
	if (data->click_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN)) {
		ret = regmap_read(data->regmap, ACT_TAP_STATUS, &click_stat);
		if (ret) {
			dev_err(data->dev,
				"error reading ACT_TAP_STATUS register\n");
			return ret;
		}
	} else
		click_stat = 0;

	/* 
	 * read the INT_SOURCE (0x30) register
	 * the click interrupt is cleared
	 */
	ret = regmap_read(data->regmap, INT_SOURCE, &int_stat);
	if (ret) {
		dev_err(data->dev, "error reading INT_SOURCE register\n");
		return ret;
	}
	/*
	 * if the SINGLE_TAP event has occurred the axl345_do_click function
	 * is called with the ACT_TAP_STATUS register as an argument
	 */
	if (int_stat & (SINGLE_TAP)) {
		dev_info(data->dev, "single click interrupt has occurred\n");

		if (click_stat & TAP_X_EN) {
			iio_push_event(
				indio_dev,
				IIO_MOD_EVENT_CODE(IIO_ACCEL, 0, IIO_MOD_X,
						   IIO_EV_TYPE_THRESH, 0),
				data->timestamp);
		}
		if (click_stat & TAP_Y_EN) {
			iio_push_event(
				indio_dev,
				IIO_MOD_EVENT_CODE(IIO_ACCEL, 0, IIO_MOD_Y,
						   IIO_EV_TYPE_THRESH, 0),
				data->timestamp);
		}
		if (click_stat & TAP_Z_EN) {
			iio_push_event(
				indio_dev,
				IIO_MOD_EVENT_CODE(IIO_ACCEL, 0, IIO_MOD_Z,
						   IIO_EV_TYPE_THRESH, 0),
				data->timestamp);
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t lsm303dlhc_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct lsm303dlhc_data *data = iio_priv(indio_dev);
	s16 buf[8]; /* 16 bytes */
	int i, ret, j = 0, base = DATAX0;
	s16 sample;

	/* read the channels that have been enabled from user space */
	for_each_set_bit(i, indio_dev->active_scan_mask,
			 indio_dev->masklength) {
		ret = regmap_bulk_read(data->regmap, base + i * sizeof(sample),
				       &sample, sizeof(sample));
		if (ret < 0)
			goto done;
		buf[j++] = sample;
	}

	/* each buffer entry line is 6 bytes + 2 bytes pad + 8 bytes timestamp */
	iio_push_to_buffers_with_timestamp(indio_dev, buf, pf->timestamp);

done:
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

int lsm303dlhc_core_probe(struct device *dev, struct regmap *regmap,
			  const char *name)
{
	struct iio_dev *indio_dev;
	struct lsm303dlhc_data *data;
	u32 regval;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	/* link private data with indio_dev */
	data = iio_priv(indio_dev);
	data->dev = dev;

	/* link spi device with indio_dev */
	dev_set_drvdata(dev, indio_dev);

	data->gpio =
		devm_gpiod_get_index(dev, LSM303DLHC_GPIO_NAME, 0, GPIOD_IN);
	if (IS_ERR(data->gpio)) {
		dev_err(dev, "gpio get index failed\n");
		return PTR_ERR(data->gpio);
	}

	data->irq = gpiod_to_irq(data->gpio);
	if (data->irq < 0)
		return data->irq;
	dev_info(dev, "The IRQ number is: %d\n", data->irq);

	/* Initialize our private device structure */
	data->regmap = regmap;
	data->data_range = HI_RESOLUTION | FS_2G;
	data->click_threshold = 50;
	data->click_duration = 3;
	data->click_axis_control = SINGLE_CLICK_Z_EN;
	data->data_rate = 0;
	data->fifo_mode = FIFO_MODE_BYPASS;

	indio_dev->dev.parent = dev;
	indio_dev->name = name;
	indio_dev->info = &lsm303dlhc_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->available_scan_masks = lsm303dlhc_accel_scan_masks;
	indio_dev->channels = lsm303dlhc_channels;
	indio_dev->num_channels = ARRAY_SIZE(lsm303dlhc_channels);

	/* Initialize the LSM303DLHC registers */

	/* 13-bit full resolution right justified */
	ret = regmap_write(data->regmap, CTRL_REG4_A, data->data_range);
	if (ret < 0)
		goto error_standby;

	/* Set the click threshold and duration */
	ret = regmap_write(data->regmap, CLICK_THS_A, data->click_threshold);
	if (ret < 0)
		goto error_standby;
	ret = regmap_write(data->regmap, TIME_LIMIT_A, data->click_duration);
	if (ret < 0)
		goto error_standby;

	/* set the axis where the click will be detected */
	ret = regmap_write(data->regmap, CLICK_CFG_A, data->click_axis_control);
	if (ret < 0)
		goto error_standby;

	/* 
         * set the data rate and the axis reading power
	 * mode, less or higher noise reducing power, in
	 * the initial settings is NO low power
	 */
	ret = regmap_write(data->regmap, CTRL_REG1_A, RATE(data->data_rate));
	if (ret < 0)
		goto error_standby;

	/* Set the FIFO mode, no FIFO by default */
	ret = regmap_write(data->regmap, FIFO_CTRL_REG_A,
			   FIFO_MODE(data->fifo_mode));
	if (ret < 0)
		goto error_standby;

	/* Map all INTs to INT1 pin */
	ret = regmap_write(data->regmap, INT_MAP, 0);
	if (ret < 0)
		goto error_standby;

	/* Enables interrupts */
	if (data->click_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))
		data->int_mask |= SINGLE_TAP;

	ret = regmap_write(data->regmap, INT_ENABLE, data->int_mask);
	if (ret < 0)
		goto error_standby;

	/* Enable measurement mode */
	ret = regmap_write(data->regmap, POWER_CTL, PCTL_MEASURE);
	if (ret < 0)
		goto error_standby;

	/* Request threaded interrupt */
	ret = devm_request_threaded_irq(dev, data->irq, NULL,
					lsm303dlhc_event_handler,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					dev_name(dev), indio_dev);
	if (ret) {
		dev_err(dev, "failed to request interrupt %d (%d)", data->irq,
			ret);
		goto error_standby;
	}

	dev_info(dev, "using interrupt %d", data->irq);

	ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
					      &iio_pollfunc_store_time,
					      lsm303dlhc_trigger_handler, NULL);
	if (ret) {
		dev_err(dev, "unable to setup triggered buffer\n");
		goto error_standby;
	}

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret) {
		dev_err(dev, "iio_device_register failed: %d\n", ret);
		goto error_standby;
	}

	return 0;

error_standby:
	dev_info(dev, "set standby mode due to an error\n");
	regmap_write(data->regmap, POWER_CTL, PCTL_STANDBY);
	return ret;
}

int lsm303dlhc_core_remove(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct lsm303dlhc_data *data = iio_priv(indio_dev);
	dev_info(data->dev, "my_remove() function is called.\n");
	return regmap_write(data->regmap, POWER_CTL, PCTL_STANDBY);
}

static int lsm303dlhc_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(client, &adxl345_i2c_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Error initializing i2c regmap: %ld\n",
			PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}
	return lsm303dlhc_core_probe(&client->dev, regmap,
				     id ? id->name : NULL);
}

static int lsm303dlhc_i2c_remove(struct spi_device *spi)
{
	return lsm303dlhc_core_remove(&spi->dev);
}

static const struct of_device_id lsm303dlhc_dt_ids[] = {
	{
		.compatible = "arrow,lsm303dlhc",
	},
	{}
};
MODULE_DEVICE_TABLE(of, lsm303dlhc_dt_ids);

static const struct spi_device_id lsm303dlhc_id[] = {
	{
		.name = "lsm303dlhc",
	},
	{}
};
MODULE_DEVICE_TABLE(spi, lsm303dlhc_id);

static struct spi_driver lsm303dlhc_driver = {
	.driver = {
		.name = "lsm303dlhc",
		.owner = THIS_MODULE,
		.of_match_table = lsm303dlhc_dt_ids,
	},
	.probe   = lsm303dlhc_i2c_probe,
	.remove  = lsm303dlhc_i2c_remove,
	.id_table	= lsm303dlhc_id,
};

module_spi_driver(lsm303dlhc_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alberto Liberal <aliberal@arroweurope.com>");
MODULE_DESCRIPTION("LSM303DLHC Three-Axis Accelerometer Regmap SPI Bus Driver");
