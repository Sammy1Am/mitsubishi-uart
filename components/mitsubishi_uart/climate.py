import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID, CONF_OUTPUT, CONF_UPDATE_INTERVAL

AUTO_LOAD = ["climate"]

CONF_HP_UART = "hp_uart"

DEFAULT_POLLING_INTERVAL = "2000ms"

mitsubishi_uart_ns = cg.esphome_ns.namespace('mitsubishi_uart')
MitsubishiUART = mitsubishi_uart_ns.class_('MitsubishiUART', climate.Climate, cg.PollingComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend({
    cv.GenerateID(CONF_ID): cv.declare_id(MitsubishiUART),
    cv.Required(CONF_HP_UART): cv.use_id(uart.UARTComponent), #TODO Set a default?
}).extend(cv.polling_component_schema(DEFAULT_POLLING_INTERVAL))

async def to_code(config):
    hp_uart_component = await cg.get_variable(config[CONF_HP_UART])
    #update_interval = await cg.get_variable(config[CONF_UPDATE_INTERVAL])
    var = cg.new_Pvariable(config[CONF_ID], hp_uart_component)

    cg.register_component(var, config)
    climate.register_climate(var, config)