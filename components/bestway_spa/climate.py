import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
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
    CONF_CIO_CLK_PIN,
    CONF_CIO_DATA_PIN,
    CONF_CIO_CS_PIN,
    CONF_DSP_DATA_PIN,
    CONF_DSP_CLK_PIN,
    CONF_DSP_CS_PIN,
    CONF_AUDIO_PIN,
    CONF_CLK_PIN,
    CONF_DATA_PIN,
    CONF_CS_PIN,
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
    CONF_BUTTON_STATUS,
    CONF_PREVENT_HIBERNATE,
)

AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

# The driver needs all MITM pins to be configurable as high-speed GPIOs
_BASE_SCHEMA = climate._CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(BestwaySpa),
    cv.Optional(CONF_PROTOCOL_TYPE, default="6WIRE_T1"): cv.enum(PROTOCOL_TYPES, upper=True),
    cv.Optional(CONF_MODEL, default="PRE2021"): cv.enum(SPA_MODELS, upper=True),
    cv.Optional(CONF_PREVENT_HIBERNATE, default=True): cv.boolean,
    
    # Use input_pin_schema for all MITM pins so C++ can handle the bit-banging direction
    cv.Optional(CONF_CIO_CLK_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_CIO_DATA_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_CIO_CS_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_DSP_DATA_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_DSP_CLK_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_DSP_CS_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_AUDIO_PIN): pins.internal_gpio_output_pin_schema,

    # Sensors
    cv.Optional(CONF_CURRENT_TEMPERATURE): sensor.sensor_schema(unit_of_measurement="°C", accuracy_decimals=0),
    cv.Optional(CONF_TARGET_TEMPERATURE): sensor.sensor_schema(unit_of_measurement="°C", accuracy_decimals=0),
    cv.Optional(CONF_HEATING): binary_sensor.binary_sensor_schema(),
    cv.Optional(CONF_FILTER): binary_sensor.binary_sensor_schema(),
    cv.Optional(CONF_BUBBLES): binary_sensor.binary_sensor_schema(),
    cv.Optional(CONF_POWER): binary_sensor.binary_sensor_schema(),
    cv.Optional(CONF_DISPLAY_TEXT): text_sensor.text_sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = _BASE_SCHEMA

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_protocol_type(config[CONF_PROTOCOL_TYPE]))

    # Mapping Pins
    for conf_key, setter in [
        (CONF_CIO_CLK_PIN, var.set_cio_clk_pin),
        (CONF_CIO_DATA_PIN, var.set_cio_data_pin),
        (CONF_CIO_CS_PIN, var.set_cio_cs_pin),
        (CONF_DSP_CLK_PIN, var.set_dsp_clk_pin),
        (CONF_DSP_DATA_PIN, var.set_dsp_data_pin),
        (CONF_DSP_CS_PIN, var.set_dsp_cs_pin),
    ]:
        if conf_key in config:
            pin = await cg.gpio_pin_expression(config[conf_key])
            cg.add(setter(pin))

    # Mapping Sensors
    if CONF_CURRENT_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_TEMPERATURE])
        cg.add(var.set_current_temperature_sensor(sens))
    if CONF_DISPLAY_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DISPLAY_TEXT])
        cg.add(var.set_display_text_sensor(sens))
