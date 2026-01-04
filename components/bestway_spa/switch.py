import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_TYPE

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

SWITCH_TYPES = {
    "heater": BestwaySpaHeaterSwitch,
    "filter": BestwaySpaFilterSwitch,
    "bubbles": BestwaySpaBubblesSwitch,
    "jets": BestwaySpaJetsSwitch,
    "lock": BestwaySpaLockSwitch,
    "power": BestwaySpaPowerSwitch,
    "unit": BestwaySpaUnitSwitch,
    "timer": BestwaySpaTimerSwitch,
    "up": BestwaySpaUpButton,
    "down": BestwaySpaDownButton,
}


def _create_switch_schema(switch_class):
    """Create a schema for a specific switch type."""
    return (
        switch.switch_schema(switch_class)
        .extend({
            cv.GenerateID(): cv.declare_id(switch_class),
            cv.GenerateID(CONF_BESTWAY_SPA_ID): cv.use_id(BestwaySpa),
        })
        .extend(cv.COMPONENT_SCHEMA)
    )


# Use typed_schema to properly declare ID types for each switch type
CONFIG_SCHEMA = cv.typed_schema({
    "heater": _create_switch_schema(BestwaySpaHeaterSwitch),
    "filter": _create_switch_schema(BestwaySpaFilterSwitch),
    "bubbles": _create_switch_schema(BestwaySpaBubblesSwitch),
    "jets": _create_switch_schema(BestwaySpaJetsSwitch),
    "lock": _create_switch_schema(BestwaySpaLockSwitch),
    "power": _create_switch_schema(BestwaySpaPowerSwitch),
    "unit": _create_switch_schema(BestwaySpaUnitSwitch),
    "timer": _create_switch_schema(BestwaySpaTimerSwitch),
    "up": _create_switch_schema(BestwaySpaUpButton),
    "down": _create_switch_schema(BestwaySpaDownButton),
})


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_BESTWAY_SPA_ID])
    cg.add(var.set_parent(parent))
