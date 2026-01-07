"""
Bestway Spa ESPHome Component

Adapted from VisualApproach WiFi-remote-for-Bestway-Lay-Z-SPA
https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA

This __init__.py provides namespace definitions and shared exports.
The actual platform implementation is in climate.py.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, switch

# No global DEPENDENCIES - UART is only needed for 4-wire protocols
# Climate.py handles protocol-specific dependencies
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

# =============================================================================
# NAMESPACE DEFINITIONS
# =============================================================================

bestway_spa_ns = cg.esphome_ns.namespace("bestway_spa")

# Main component class (does NOT inherit from UARTDevice - that's handled conditionally)
BestwaySpa = bestway_spa_ns.class_("BestwaySpa", climate.Climate, cg.Component)

# Protocol types
ProtocolType = bestway_spa_ns.enum("ProtocolType")
PROTOCOL_TYPES = {
    "4WIRE": ProtocolType.PROTOCOL_4WIRE,
    "6WIRE_T1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_T2": ProtocolType.PROTOCOL_6WIRE_T2,
    # Aliases for convenience
    "6WIRE": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_TYPE1": ProtocolType.PROTOCOL_6WIRE_T1,
    "6WIRE_TYPE2": ProtocolType.PROTOCOL_6WIRE_T2,
}

# Spa models
SpaModel = bestway_spa_ns.enum("SpaModel")
SPA_MODELS = {
    "PRE2021": SpaModel.MODEL_PRE2021,
    "54149E": SpaModel.MODEL_54149E,
    "54123": SpaModel.MODEL_54123,
    "54138": SpaModel.MODEL_54138,
    "54144": SpaModel.MODEL_54144,
    "54154": SpaModel.MODEL_54154,
    "54173": SpaModel.MODEL_54173,
    "P05504": SpaModel.MODEL_P05504,
}

# =============================================================================
# CONFIGURATION KEY EXPORTS
# =============================================================================

CONF_BESTWAY_SPA_ID = "bestway_spa_id"
CONF_PROTOCOL_TYPE = "protocol_type"
CONF_MODEL = "model"

# CIO bus pins (input from pump controller)
CONF_CIO_CLK_PIN = "cio_clk_pin"
CONF_CIO_DATA_PIN = "cio_data_pin"
CONF_CIO_CS_PIN = "cio_cs_pin"

# DSP bus pins (to physical display panel)
CONF_DSP_DATA_PIN = "dsp_data_pin"
CONF_DSP_CLK_PIN = "dsp_clk_pin"
CONF_DSP_CS_PIN = "dsp_cs_pin"
CONF_AUDIO_PIN = "audio_pin"

# Legacy pin names (for backwards compatibility)
CONF_CLK_PIN = "clk_pin"
CONF_DATA_PIN = "data_pin"
CONF_CS_PIN = "cs_pin"

# Sensor configuration keys
CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_HEATING = "heating"
CONF_FILTER = "filter"
CONF_BUBBLES = "bubbles"
CONF_JETS = "jets"
CONF_LOCKED = "locked"
CONF_POWER = "power"
CONF_ERROR = "error"
CONF_ERROR_TEXT = "error_text"
CONF_DISPLAY_TEXT = "display_text"
CONF_BUTTON_STATUS = "button_status"

# Anti-hibernate feature
CONF_PREVENT_HIBERNATE = "prevent_hibernate"
