import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
# This is the secret sauce for 2025.12.5
from esphome.components.climate import CLIMATE_SCHEMA
from esphome.const import CONF_ID
from esphome import pins
from . import bestway_spa_ns, BestwaySpa

# Link to the ProtocolType enum in your C++
ProtocolType = bestway_spa_ns.enum("ProtocolType")
PROTOCOL_TYPES = {
    "6WIRE_T1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_T2": ProtocolType.PROTOCOL_6WIRE_T2,
    "4WIRE": ProtocolType.PROTOCOL_4WIRE,
}

# Now we use the imported CLIMATE_SCHEMA directly
CONFIG_SCHEMA = CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(BestwaySpa),
        cv.Required("protocol_type"): cv.enum(PROTOCOL_TYPES, upper=True),
        # CIO Pins
        cv.Required("cio_data_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
        cv.Required("cio_clk_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
        cv.Required("cio_cs_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
        # DSP Pins
        cv.Optional("dsp_data_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
        cv.Optional("dsp_clk_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
        cv.Optional("dsp_cs_pin"): pins.gpio_pin_schema(default_mode="INPUT"),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_protocol_type(config["protocol_type"]))
    
    # CIO Pin Setup
    cio_data = await cg.gpio_pin_expression(config["cio_data_pin"])
    cg.add(var.set_cio_data_pin(cio_data))
    cio_clk = await cg.gpio_pin_expression(config["cio_clk_pin"])
    cg.add(var.set_cio_clk_pin(cio_clk))
    cio_cs = await cg.gpio_pin_expression(config["cio_cs_pin"])
    cg.add(var.set_cio_cs_pin(cio_cs))

    # DSP Pin Setup
    if "dsp_data_pin" in config:
        dsp_data = await cg.gpio_pin_expression(config["dsp_data_pin"])
        cg.add(var.set_dsp_data_pin(dsp_data))
    if "dsp_clk_pin" in config:
        dsp_clk = await cg.gpio_pin_expression(config["dsp_clk_pin"])
        cg.add(var.set_dsp_clk_pin(dsp_clk))
    if "dsp_cs_pin" in config:
        dsp_cs = await cg.gpio_pin_expression(config["dsp_cs_pin"])
        cg.add(var.set_dsp_cs_pin(dsp_cs))
