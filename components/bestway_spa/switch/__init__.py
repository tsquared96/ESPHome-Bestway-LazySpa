import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import bestway_spa_ns, BestwaySpa, CONF_BESTWAY_SPA_ID

# Switch class
BestwaySpaSwitch = bestway_spa_ns.class_("BestwaySpaSwitch", switch.Switch, cg.Component)

# Switch type enum
SwitchType = bestway_spa_ns.enum("SwitchType")

CONF_TYPE = "type"

SWITCH_TYPES = {
    "heater": SwitchType.SWITCH_HEATER,
    "filter": SwitchType.SWITCH_FILTER,
    "bubbles": SwitchType.SWITCH_BUBBLES,
    "jets": SwitchType.SWITCH_JETS,
    "lock": SwitchType.SWITCH_LOCK,
    "power": SwitchType.SWITCH_POWER,
}

CONFIG_SCHEMA = (
    switch.switch_schema(BestwaySpaSwitch)
    .extend(
        {
            cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
            cv.Required(CONF_TYPE): cv.enum(SWITCH_TYPES, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_switch_type(config[CONF_TYPE]))
