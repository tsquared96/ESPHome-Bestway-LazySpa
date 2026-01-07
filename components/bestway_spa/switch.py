import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from . import bestway_spa_ns, BestwaySpa, CONF_BESTWAY_SPA_ID

# Switch classes
BestwaySpaHeaterSwitch = bestway_spa_ns.class_("BestwaySpaHeaterSwitch", switch.Switch, cg.Component)
BestwaySpaFilterSwitch = bestway_spa_ns.class_("BestwaySpaFilterSwitch", switch.Switch, cg.Component)
BestwaySpaBubblesSwitch = bestway_spa_ns.class_("BestwaySpaBubblesSwitch", switch.Switch, cg.Component)
BestwaySpaJetsSwitch = bestway_spa_ns.class_("BestwaySpaJetsSwitch", switch.Switch, cg.Component)
BestwaySpaLockSwitch = bestway_spa_ns.class_("BestwaySpaLockSwitch", switch.Switch, cg.Component)
BestwaySpaPowerSwitch = bestway_spa_ns.class_("BestwaySpaPowerSwitch", switch.Switch, cg.Component)
BestwaySpaUnitSwitch = bestway_spa_ns.class_("BestwaySpaUnitSwitch", switch.Switch, cg.Component)
BestwaySpaTimerSwitch = bestway_spa_ns.class_("BestwaySpaTimerSwitch", switch.Switch, cg.Component)
BestwaySpaUpButton = bestway_spa_ns.class_("BestwaySpaUpButton", switch.Switch, cg.Component)
BestwaySpaDownButton = bestway_spa_ns.class_("BestwaySpaDownButton", switch.Switch, cg.Component)

# Button enable/disable switches
BestwaySpaHeaterBtnEnabled = bestway_spa_ns.class_("BestwaySpaHeaterBtnEnabled", switch.Switch, cg.Component)
BestwaySpaFilterBtnEnabled = bestway_spa_ns.class_("BestwaySpaFilterBtnEnabled", switch.Switch, cg.Component)
BestwaySpaBubblesBtnEnabled = bestway_spa_ns.class_("BestwaySpaBubblesBtnEnabled", switch.Switch, cg.Component)
BestwaySpaJetsBtnEnabled = bestway_spa_ns.class_("BestwaySpaJetsBtnEnabled", switch.Switch, cg.Component)
BestwaySpaLockBtnEnabled = bestway_spa_ns.class_("BestwaySpaLockBtnEnabled", switch.Switch, cg.Component)
BestwaySpaPowerBtnEnabled = bestway_spa_ns.class_("BestwaySpaPowerBtnEnabled", switch.Switch, cg.Component)
BestwaySpaUnitBtnEnabled = bestway_spa_ns.class_("BestwaySpaUnitBtnEnabled", switch.Switch, cg.Component)
BestwaySpaTimerBtnEnabled = bestway_spa_ns.class_("BestwaySpaTimerBtnEnabled", switch.Switch, cg.Component)
BestwaySpaUpBtnEnabled = bestway_spa_ns.class_("BestwaySpaUpBtnEnabled", switch.Switch, cg.Component)
BestwaySpaDownBtnEnabled = bestway_spa_ns.class_("BestwaySpaDownBtnEnabled", switch.Switch, cg.Component)

# Configuration keys
CONF_HEATER = "heater"
CONF_FILTER = "filter"
CONF_BUBBLES = "bubbles"
CONF_JETS = "jets"
CONF_LOCK = "lock"
CONF_POWER = "power"
CONF_UNIT = "unit"
CONF_TIMER = "timer"
CONF_UP = "up"
CONF_DOWN = "down"
CONF_HEATER_BTN_ENABLED = "heater_btn_enabled"
CONF_FILTER_BTN_ENABLED = "filter_btn_enabled"
CONF_BUBBLES_BTN_ENABLED = "bubbles_btn_enabled"
CONF_JETS_BTN_ENABLED = "jets_btn_enabled"
CONF_LOCK_BTN_ENABLED = "lock_btn_enabled"
CONF_POWER_BTN_ENABLED = "power_btn_enabled"
CONF_UNIT_BTN_ENABLED = "unit_btn_enabled"
CONF_TIMER_BTN_ENABLED = "timer_btn_enabled"
CONF_UP_BTN_ENABLED = "up_btn_enabled"
CONF_DOWN_BTN_ENABLED = "down_btn_enabled"
CONF_SPA_ID = "spa_id"

# Multi-switch platform schema
# Usage:
#   switch:
#     - platform: bestway_spa
#       spa_id: spa
#       heater:
#         name: "Heater"
#       filter:
#         name: "Filter"
#       # jets: omitted if model doesn't have jets

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_SPA_ID): cv.use_id(BestwaySpa),

    # Control switches (only configure what your model has)
    cv.Optional(CONF_HEATER): switch.switch_schema(BestwaySpaHeaterSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_FILTER): switch.switch_schema(BestwaySpaFilterSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_BUBBLES): switch.switch_schema(BestwaySpaBubblesSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_JETS): switch.switch_schema(BestwaySpaJetsSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_LOCK): switch.switch_schema(BestwaySpaLockSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_POWER): switch.switch_schema(BestwaySpaPowerSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_UNIT): switch.switch_schema(BestwaySpaUnitSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_TIMER): switch.switch_schema(BestwaySpaTimerSwitch).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_UP): switch.switch_schema(BestwaySpaUpButton).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_DOWN): switch.switch_schema(BestwaySpaDownButton).extend(cv.COMPONENT_SCHEMA),

    # Button enable/disable switches
    cv.Optional(CONF_HEATER_BTN_ENABLED): switch.switch_schema(BestwaySpaHeaterBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_FILTER_BTN_ENABLED): switch.switch_schema(BestwaySpaFilterBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_BUBBLES_BTN_ENABLED): switch.switch_schema(BestwaySpaBubblesBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_JETS_BTN_ENABLED): switch.switch_schema(BestwaySpaJetsBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_LOCK_BTN_ENABLED): switch.switch_schema(BestwaySpaLockBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_POWER_BTN_ENABLED): switch.switch_schema(BestwaySpaPowerBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_UNIT_BTN_ENABLED): switch.switch_schema(BestwaySpaUnitBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_TIMER_BTN_ENABLED): switch.switch_schema(BestwaySpaTimerBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_UP_BTN_ENABLED): switch.switch_schema(BestwaySpaUpBtnEnabled).extend(cv.COMPONENT_SCHEMA),
    cv.Optional(CONF_DOWN_BTN_ENABLED): switch.switch_schema(BestwaySpaDownBtnEnabled).extend(cv.COMPONENT_SCHEMA),
})


async def _register_switch(config, key, switch_class, parent):
    """Helper to register a switch if configured."""
    if key not in config:
        return

    conf = config[key]
    var = cg.new_Pvariable(conf[CONF_ID])
    await cg.register_component(var, conf)
    await switch.register_switch(var, conf)
    cg.add(var.set_parent(parent))


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPA_ID])

    # Register control switches
    await _register_switch(config, CONF_HEATER, BestwaySpaHeaterSwitch, parent)
    await _register_switch(config, CONF_FILTER, BestwaySpaFilterSwitch, parent)
    await _register_switch(config, CONF_BUBBLES, BestwaySpaBubblesSwitch, parent)
    await _register_switch(config, CONF_JETS, BestwaySpaJetsSwitch, parent)
    await _register_switch(config, CONF_LOCK, BestwaySpaLockSwitch, parent)
    await _register_switch(config, CONF_POWER, BestwaySpaPowerSwitch, parent)
    await _register_switch(config, CONF_UNIT, BestwaySpaUnitSwitch, parent)
    await _register_switch(config, CONF_TIMER, BestwaySpaTimerSwitch, parent)
    await _register_switch(config, CONF_UP, BestwaySpaUpButton, parent)
    await _register_switch(config, CONF_DOWN, BestwaySpaDownButton, parent)

    # Register button enable switches
    await _register_switch(config, CONF_HEATER_BTN_ENABLED, BestwaySpaHeaterBtnEnabled, parent)
    await _register_switch(config, CONF_FILTER_BTN_ENABLED, BestwaySpaFilterBtnEnabled, parent)
    await _register_switch(config, CONF_BUBBLES_BTN_ENABLED, BestwaySpaBubblesBtnEnabled, parent)
    await _register_switch(config, CONF_JETS_BTN_ENABLED, BestwaySpaJetsBtnEnabled, parent)
    await _register_switch(config, CONF_LOCK_BTN_ENABLED, BestwaySpaLockBtnEnabled, parent)
    await _register_switch(config, CONF_POWER_BTN_ENABLED, BestwaySpaPowerBtnEnabled, parent)
    await _register_switch(config, CONF_UNIT_BTN_ENABLED, BestwaySpaUnitBtnEnabled, parent)
    await _register_switch(config, CONF_TIMER_BTN_ENABLED, BestwaySpaTimerBtnEnabled, parent)
    await _register_switch(config, CONF_UP_BTN_ENABLED, BestwaySpaUpBtnEnabled, parent)
    await _register_switch(config, CONF_DOWN_BTN_ENABLED, BestwaySpaDownBtnEnabled, parent)
