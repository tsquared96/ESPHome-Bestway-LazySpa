import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

# Define the namespace
bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")

# FIX: Added climate.Climate to the inheritance list
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", climate.Climate, cg.Component)

# No schema here, it's all in climate.py
