import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

from . import (
    bestway_spa_ns,
    BestwaySpa,
    PROTOCOL_TYPES,
    SPA_MODELS,
    CONF_PROTOCOL_TYPE,
    CONF_MODEL,
    # CIO bus pins (input)
    CONF_CIO_CLK_PIN,
    CONF_CIO_DATA_PIN,
    CONF_CIO_CS_PIN,
    # DSP bus pins (output)
    CONF_DSP_DATA_PIN,
    CONF_AUDIO_PIN,
    # Legacy pin names
    CONF_CLK_PIN,
    CONF_DATA_PIN,
    CONF_CS_PIN,
    # Sensor configs
    CONF_CURRENT_TEMPERATURE,
    CONF_TARGET_TEMPERATURE,
    CONF_HEATING,
    CONF_FILTER,
    CONF_BUBBLES,
    CONF_JETS,
    CONF_LOCKED,
    CONF_POWER,
    CONF_ERROR,
    CONF_ERROR_TEXT,
    CONF_DISPLAY_TEXT,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]


def validate_6wire_pins(config):
    """Validate that 6-wire protocols have required pins configured."""
    protocol = config.get(CONF_PROTOCOL_TYPE, "4WIRE")
    if protocol in ["6WIRE", "6WIRE_T1", "6WIRE_T2", "6WIRE_TYPE1", "6WIRE_TYPE2"]:
        # Check for new dual-bus pins first, then fall back to legacy names
        has_cio_clk = CONF_CIO_CLK_PIN in config or CONF_CLK_PIN in config
        has_cio_data = CONF_CIO_DATA_PIN in config or CONF_DATA_PIN in config
        has_cio_cs = CONF_CIO_CS_PIN in config or CONF_CS_PIN in config

        if not has_cio_clk:
            raise cv.Invalid("cio_clk_pin (or clk_pin) is required for 6-wire protocols")
        if not has_cio_data:
            raise cv.Invalid("cio_data_pin (or data_pin) is required for 6-wire protocols")
        if not has_cio_cs:
            raise cv.Invalid("cio_cs_pin (or cs_pin) is required for 6-wire protocols")
    return config


# Use climate._CLIMATE_SCHEMA for ESPHome 2024+ (CLIMATE_SCHEMA is now private)
CONFIG_SCHEMA = cv.All(
    climate._CLIMATE_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(BestwaySpa),

        # Protocol configuration
        cv.Optional(CONF_PROTOCOL_TYPE, default="4WIRE"): cv.enum(PROTOCOL_TYPES, upper=True),
        cv.Optional(CONF_MODEL, default="54154"): cv.enum(SPA_MODELS, upper=True),

        # 6-wire MITM dual-bus pin configuration
        # CIO bus pins (input from pump controller)
        cv.Optional(CONF_CIO_CLK_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_CIO_DATA_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_CIO_CS_PIN): pins.internal_gpio_input_pin_schema,
        # DSP bus pins (output to pump controller)
        cv.Optional(CONF_DSP_DATA_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_AUDIO_PIN): pins.internal_gpio_output_pin_schema,
        # Legacy pin names (for backwards compatibility)
        cv.Optional(CONF_CLK_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_DATA_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_CS_PIN): pins.internal_gpio_input_pin_schema,

        # Temperature sensors
        cv.Optional(CONF_CURRENT_TEMPERATURE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_TARGET_TEMPERATURE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),

        # Binary sensors
        cv.Optional(CONF_HEATING): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_FILTER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_BUBBLES): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_JETS): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_LOCKED): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_POWER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_ERROR): binary_sensor.binary_sensor_schema(),

        # Text sensors
        cv.Optional(CONF_ERROR_TEXT): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_DISPLAY_TEXT): text_sensor.text_sensor_schema(),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    validate_6wire_pins,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    # Set protocol type
    cg.add(var.set_protocol_type(config[CONF_PROTOCOL_TYPE]))

    # Set model
    cg.add(var.set_model(config[CONF_MODEL]))

    # Configure 6-wire MITM dual-bus pins
    # CIO bus pins (input from pump controller)
    if CONF_CIO_CLK_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_CIO_CLK_PIN])
        cg.add(var.set_cio_clk_pin(pin))
    elif CONF_CLK_PIN in config:  # Legacy fallback
        pin = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
        cg.add(var.set_cio_clk_pin(pin))

    if CONF_CIO_DATA_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_CIO_DATA_PIN])
        cg.add(var.set_cio_data_pin(pin))
    elif CONF_DATA_PIN in config:  # Legacy fallback
        pin = await cg.gpio_pin_expression(config[CONF_DATA_PIN])
        cg.add(var.set_cio_data_pin(pin))

    if CONF_CIO_CS_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_CIO_CS_PIN])
        cg.add(var.set_cio_cs_pin(pin))
    elif CONF_CS_PIN in config:  # Legacy fallback
        pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
        cg.add(var.set_cio_cs_pin(pin))

    # DSP bus pins (output to pump controller)
    if CONF_DSP_DATA_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_DSP_DATA_PIN])
        cg.add(var.set_dsp_data_pin(pin))

    if CONF_AUDIO_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_AUDIO_PIN])
        cg.add(var.set_audio_pin(pin))

    # Register temperature sensors
    if CONF_CURRENT_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_TEMPERATURE])
        cg.add(var.set_current_temperature_sensor(sens))

    if CONF_TARGET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TARGET_TEMPERATURE])
        cg.add(var.set_target_temperature_sensor(sens))

    # Register binary sensors
    if CONF_HEATING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_HEATING])
        cg.add(var.set_heating_sensor(sens))

    if CONF_FILTER in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_FILTER])
        cg.add(var.set_filter_sensor(sens))

    if CONF_BUBBLES in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BUBBLES])
        cg.add(var.set_bubbles_sensor(sens))

    if CONF_JETS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_JETS])
        cg.add(var.set_jets_sensor(sens))

    if CONF_LOCKED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LOCKED])
        cg.add(var.set_locked_sensor(sens))

    if CONF_POWER in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))

    if CONF_ERROR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ERROR])
        cg.add(var.set_error_sensor(sens))

    # Register text sensors
    if CONF_ERROR_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ERROR_TEXT])
        cg.add(var.set_error_text_sensor(sens))

    if CONF_DISPLAY_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DISPLAY_TEXT])
        cg.add(var.set_display_text_sensor(sens))
