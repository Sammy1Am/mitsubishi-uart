import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import (
    CONF_ID,
)
from esphome.core import coroutine

AUTO_LOAD = ["climate", "select", "sensor"]
DEPENDENCIES = ["uart", "climate"]

CONF_HP_UART = "hp_uart"

DEFAULT_POLLING_INTERVAL = "5s"

mitsubishi_uart_ns = cg.esphome_ns.namespace("mitsubishi_uart")
MitsubishiUART = mitsubishi_uart_ns.class_("MitsubishiUART", cg.PollingComponent, climate.Climate)

CONFIG_SCHEMA = cv.polling_component_schema(DEFAULT_POLLING_INTERVAL).extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(MitsubishiUART),
        cv.Required(CONF_HP_UART): cv.use_id(uart.UARTComponent),  # TODO Set a default?
    }
)


@coroutine
async def to_code(config):
    hp_uart_component = await cg.get_variable(config[CONF_HP_UART])
    var = cg.new_Pvariable(config[CONF_ID], hp_uart_component)

    await cg.register_component(var, config)
