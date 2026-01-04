import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import bestway_spa_ns, BestwaySpa, CONF_BESTWAY_SPA_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

# Protocol types
ProtocolType = bestway_spa_ns.enum("ProtocolType")
PROTOCOL_TYPES = {
    "4WIRE": ProtocolType.PROTOCOL_4WIRE,
    "6WIRE_T1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_T2": ProtocolType.PROTOCOL_6WIRE_T2,
    "6WIRE": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_TYPE1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_TYPE2": ProtocolType.PROTOCOL_6WIRE_T2,
}

# Spa models
SpaModel = bestway_spa_ns.enum("SpaModel")
SPA_MODELS = {
    "PRE2021": SpaModel.MODEL_PRE2021,
    "54149E": SpaModel.MODEL_54149E,
    "54123": SpaModel.MODEL_54123,
    "54138": SpaModel.MODEL_54138,
    "54144": SpaModel.MODEL_54144,
    "54154": SpaModel.MODEL_54154,
    "54173": SpaModel.MODEL_54173,
    "P05504": SpaModel.MODEL_P05504,
    "MODEL_PRE2021": SpaModel.MODEL_PRE2021,
    "MODEL_54149E": SpaModel.MODEL_54149E,
    "NO54123": SpaModel.MODEL_54123,
    "NO54138": SpaModel.MODEL_54138,
    "NO54144": SpaModel.MODEL_54144,
    "NO54154": SpaModel.MODEL_54154,
    "NO54173": SpaModel.MODEL_54173,
}

# Configuration keys
CONF_PROTOCOL_TYPE = "protocol_type"
CONF_MODEL = "model"
CONF_CLK_PIN = "clk_pin"
CONF_DATA_PIN = "data_pin"
CONF_CS_PIN = "cs_pin"
CONF_AUDIO_PIN = "audio_pin"
CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_HEATING = "heating"
CONF_FILTER = "filter"
CONF_BUBBLES = "bubbles"
CONF_JETS = "jets"
CONF_LOCKED = "locked"
CONF_POWER = "power"
CONF_ERROR = "error"
CONF_ERROR_TEXT = "error_text"
CONF_DISPLAY_TEXT = "display_text"


def validate_6wire_pins(config):
    """Validate that 6-wire protocols have required pins configured."""
    protocol = str(config.get(CONF_PROTOCOL_TYPE, "4WIRE")).upper()
    if "6WIRE" in protocol:
        if CONF_CLK_PIN not in config:
            raise cv.Invalid("clk_pin is required for 6-wire protocols")
        if CONF_DATA_PIN not in config:
            raise cv.Invalid("data_pin is required for 6-wire protocols")
        if CONF_CS_PIN not in config:
            raise cv.Invalid("cs_pin is required for 6-wire protocols")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BestwaySpa),

            # Protocol configuration
            cv.Optional(CONF_PROTOCOL_TYPE, default="4WIRE"): cv.enum(PROTOCOL_TYPES, upper=True),
            cv.Optional(CONF_MODEL, default="54154"): cv.enum(SPA_MODELS, upper=True),

            # 6-wire pin configuration (we set modes manually in C++)
            cv.Optional(CONF_CLK_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_DATA_PIN): pins.internal_gpio_output_pin_schema,  # Bidirectional, set manually
            cv.Optional(CONF_CS_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_AUDIO_PIN): pins.gpio_output_pin_schema,

            # Temperature sensors
            cv.Optional(CONF_CURRENT_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=1,
            ),
            cv.Optional(CONF_TARGET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
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
        }
    )
    .extend(climate._CLIMATE_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    validate_6wire_pins,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)

    # Set protocol type
    cg.add(var.set_protocol_type(config[CONF_PROTOCOL_TYPE]))

    # Set model
    cg.add(var.set_model(config[CONF_MODEL]))

    # Configure 6-wire pins
    if CONF_CLK_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
        cg.add(var.set_clk_pin(pin))

    if CONF_DATA_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_DATA_PIN])
        cg.add(var.set_data_pin(pin))

    if CONF_CS_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
        cg.add(var.set_cs_pin(pin))

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
