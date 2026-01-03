import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart

CODEOWNERS = ["@tsquared96"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["climate", "sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = True

# Namespace
bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")

# Main component class
BestwaySpa = bestway_spa_ns.class_(
    "BestwaySpa", climate.Climate, uart.UARTDevice, cg.Component
)

# Configuration key for referencing parent
CONF_BESTWAY_SPA_ID = "bestway_spa_id"

# Empty config schema since actual config is in climate.py
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
