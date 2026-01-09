import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", cg.Component)

CONF_BESTWAY_SPA_ID = "bestway_spa_id"

CONFIG_SCHEMA = cv.Schema({})
