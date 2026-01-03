import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor, switch
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_FAHRENHEIT,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["climate", "sensor", "binary_sensor", "text_sensor"]

bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", climate.Climate, uart.UARTDevice, cg.Component)

# Protocol types
ProtocolType = bestway_spa_ns.enum("ProtocolType")
PROTOCOL_TYPES = {
    "4WIRE": ProtocolType.PROTOCOL_4WIRE,
    "6WIRE": ProtocolType.PROTOCOL_6WIRE,
}

# Switch classes
BestwaySpaHeaterSwitch = bestway_spa_ns.class_("BestwaySpaHeaterSwitch", switch.Switch, cg.Component)
BestwaySpaFilterSwitch = bestway_spa_ns.class_("BestwaySpaFilterSwitch", switch.Switch, cg.Component)
BestwaySpaBubblesSwitch = bestway_spa_ns.class_("BestwaySpaBubblesSwitch", switch.Switch, cg.Component)
BestwaySpaLockSwitch = bestway_spa_ns.class_("BestwaySpaLockSwitch", switch.Switch, cg.Component)

CONF_BESTWAY_SPA_ID = "bestway_spa_id"
CONF_PROTOCOL_TYPE = "protocol_type"
CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_HEATING = "heating"
CONF_FILTER = "filter"
CONF_BUBBLES = "bubbles"
CONF_LOCKED = "locked"
CONF_ERROR = "error"
CONF_ERROR_TEXT = "error_text"

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(BestwaySpa),
            cv.Optional(CONF_PROTOCOL_TYPE, default="4WIRE"): cv.enum(PROTOCOL_TYPES, upper=True),
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
            cv.Optional(CONF_HEATING): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_FILTER): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_BUBBLES): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_LOCKED): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_ERROR): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_ERROR_TEXT): text_sensor.text_sensor_schema(),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
    
    # Set protocol type
    cg.add(var.set_protocol_type(config[CONF_PROTOCOL_TYPE]))
    
    # Register sensors
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
    
    if CONF_LOCKED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LOCKED])
        cg.add(var.set_locked_sensor(sens))
    
    if CONF_ERROR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ERROR])
        cg.add(var.set_error_sensor(sens))
    
    # Register text sensor
    if CONF_ERROR_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_ERROR_TEXT])
        cg.add(var.set_error_text_sensor(sens))


# Switch platforms
BESTWAY_SPA_SWITCH_SCHEMA = switch.switch_schema().extend({
    cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
})


async def register_bestway_switch(var, config):
    await switch.register_switch(var, config)
    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    cg.add(var.set_parent(parent))


@cv.validate_registry("switch", "bestway_spa_heater")
async def heater_switch_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], BestwaySpaHeaterSwitch())
    await register_bestway_switch(var, config)


@cv.validate_registry("switch", "bestway_spa_filter")
async def filter_switch_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], BestwaySpaFilterSwitch())
    await register_bestway_switch(var, config)


@cv.validate_registry("switch", "bestway_spa_bubbles")
async def bubbles_switch_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], BestwaySpaBubblesSwitch())
    await register_bestway_switch(var, config)


@cv.validate_registry("switch", "bestway_spa_lock")
async def lock_switch_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], BestwaySpaLockSwitch())
    await register_bestway_switch(var, config)
