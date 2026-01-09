import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import BestwaySpa, CONF_BESTWAY_SPA_ID

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
    cv.Optional("display_text"): text_sensor.text_sensor_schema(),
})

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    if "display_text" in config:
        sens = await text_sensor.new_text_sensor(config["display_text"])
        cg.add(parent.set_display_text_sensor(sens))
