import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import BestwaySpa, CONF_BESTWAY_SPA_ID

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
    cv.Optional("heating_sensor"): binary_sensor.binary_sensor_schema(),
    cv.Optional("filter_sensor"): binary_sensor.binary_sensor_schema(),
    cv.Optional("bubbles_sensor"): binary_sensor.binary_sensor_schema(),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    if "heating_sensor" in config:
        sens = await binary_sensor.new_binary_sensor(config["heating_sensor"])
        cg.add(parent.set_heating_sensor(sens))
    if "filter_sensor" in config:
        sens = await binary_sensor.new_binary_sensor(config["filter_sensor"])
        cg.add(parent.set_filter_sensor(sens))
    if "bubbles_sensor" in config:
        sens = await binary_sensor.new_binary_sensor(config["bubbles_sensor"])
        cg.add(parent.set_bubbles_sensor(sens))
