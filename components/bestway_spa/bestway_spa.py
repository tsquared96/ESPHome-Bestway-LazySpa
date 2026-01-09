import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, sensor, binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_MODEL,
)

# Define the namespace
bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", cg.Component, climate.Climate)

# Define the Enums for the YAML
ProtocolType = bestway_spa_ns.enum("ProtocolType")
PROTOCOL_OPTIONS = {
    "4WIRE": ProtocolType.PROTOCOL_4WIRE,
    "6WIRE_T1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_T2": ProtocolType.PROTOCOL_6WIRE_T2,
}

CONF_PROTOCOL = "protocol"
CONF_CIO_DATA_PIN = "cio_data_pin"
CONF_CIO_CLK_PIN = "cio_clk_pin"
CONF_CIO_CS_PIN = "cio_cs_pin"
CONF_DSP_DATA_PIN = "dsp_data_pin"
CONF_DSP_CLK_PIN = "dsp_clk_pin"
CONF_DSP_CS_PIN = "dsp_cs_pin"

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(BestwaySpa),
            cv.Required(CONF_PROTOCOL): cv.enum(PROTOCOL_OPTIONS, upper=True),
            # Pins for the Pump Controller (CIO)
            cv.Required(CONF_CIO_DATA_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_CIO_CLK_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_CIO_CS_PIN): pins.gpio_input_pin_schema,
            # Pins for the Physical Display (DSP) - Optional
            cv.Optional(CONF_DSP_DATA_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_DSP_CLK_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_DSP_CS_PIN): pins.gpio_output_pin_schema,
        }
    ).extend(cv.COMPONENT_SCHEMA),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_protocol_type(config[CONF_PROTOCOL]))

    # Set CIO pins
    cio_data = await cg.gpio_pin_expression(config[CONF_CIO_DATA_PIN])
    cg.add(var.set_cio_data_pin(cio_data))
    cio_clk = await cg.gpio_pin_expression(config[CONF_CIO_CLK_PIN])
    cg.add(var.set_cio_clk_pin(cio_clk))
    cio_cs = await cg.gpio_pin_expression(config[CONF_CIO_CS_PIN])
    cg.add(var.set_cio_cs_pin(cio_cs))

    # Set DSP pins if they exist
    if CONF_DSP_DATA_PIN in config:
        dsp_data = await cg.gpio_pin_expression(config[CONF_DSP_DATA_PIN])
        cg.add(var.set_dsp_data_pin(dsp_data))
    if CONF_DSP_CLK_PIN in config:
        dsp_clk = await cg.gpio_pin_expression(config[CONF_DSP_CLK_PIN])
        cg.add(var.set_dsp_clk_pin(dsp_clk))
    if CONF_DSP_CS_PIN in config:
        dsp_cs = await cg.gpio_pin_expression(config[CONF_DSP_CS_PIN])
        cg.add(var.set_dsp_cs_pin(dsp_cs))
