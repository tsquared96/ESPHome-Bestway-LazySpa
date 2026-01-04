import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_TYPE

from . import bestway_spa_ns, BestwaySpa, CONF_BESTWAY_SPA_ID

DEPENDENCIES = ["bestway_spa"]

# Switch classes
BestwaySpaHeaterSwitch = bestway_spa_ns.class_("BestwaySpaHeaterSwitch", switch.Switch, cg.Component)
BestwaySpaFilterSwitch = bestway_spa_ns.class_("BestwaySpaFilterSwitch", switch.Switch, cg.Component)
BestwaySpaBubblesSwitch = bestway_spa_ns.class_("BestwaySpaBubblesSwitch", switch.Switch, cg.Component)
BestwaySpaJetsSwitch = bestway_spa_ns.class_("BestwaySpaJetsSwitch", switch.Switch, cg.Component)
BestwaySpaLockSwitch = bestway_spa_ns.class_("BestwaySpaLockSwitch", switch.Switch, cg.Component)
BestwaySpaPowerSwitch = bestway_spa_ns.class_("BestwaySpaPowerSwitch", switch.Switch, cg.Component)

SWITCH_TYPES = {
    "heater": BestwaySpaHeaterSwitch,
    "filter": BestwaySpaFilterSwitch,
    "bubbles": BestwaySpaBubblesSwitch,
    "jets": BestwaySpaJetsSwitch,
    "lock": BestwaySpaLockSwitch,
    "power": BestwaySpaPowerSwitch,
}

CONFIG_SCHEMA = (
    switch.switch_schema(switch.Switch)
    .extend({
        cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
        cv.Required(CONF_TYPE): cv.one_of(*SWITCH_TYPES, lower=True),
    })
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    switch_type = config[CONF_TYPE]
    switch_class = SWITCH_TYPES[switch_type]

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    cg.add(var.set_parent(parent))
