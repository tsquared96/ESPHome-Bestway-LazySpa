import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

# Define the namespace
bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")

# FIX: Class registration for ESPHome 2025.12.5
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", climate.Climate, cg.Component)

# FIX: Constant required by sensor.py and binary_sensor.py
CONF_BESTWAY_SPA_ID = "bestway_spa_id"
