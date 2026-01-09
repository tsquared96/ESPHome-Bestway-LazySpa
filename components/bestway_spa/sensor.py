import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from . import bestway_spa_ns, BestwaySpa, CONF_BESTWAY_SPA_ID

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
    cv.Optional("current_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("target_temperature"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    if "current_temperature" in config:
        sens = await sensor.new_sensor(config["current_temperature"])
        cg.add(parent.set_current_temperature_sensor(sens))
    if "target_temperature" in config:
        sens = await sensor.new_sensor(config["target_temperature"])
        cg.add(parent.set_target_temperature_sensor(sens))
