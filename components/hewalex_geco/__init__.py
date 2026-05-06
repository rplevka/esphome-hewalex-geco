import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@romanplevka"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

hewalex_geco_ns = cg.esphome_ns.namespace("hewalex_geco")
HewalexGecoHub = hewalex_geco_ns.class_(
    "HewalexGecoHub", cg.PollingComponent, uart.UARTDevice
)

CONF_HEWALEX_GECO_ID = "hewalex_geco_id"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"
CONF_TARGET_ADDRESS = "target_address"
CONF_SOURCE_ADDRESS = "source_address"
CONF_FUNCTION_CODE = "function_code"
CONF_REGISTER_RANGES = "register_ranges"
CONF_START = "start"
CONF_COUNT = "count"

REGISTER_RANGE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_START): cv.uint16_t,
        cv.Required(CONF_COUNT): cv.int_range(min=1, max=200),
    }
)

DEFAULT_RANGES = [
    {CONF_START: 100, CONF_COUNT: 50},
    {CONF_START: 150, CONF_COUNT: 50},
    {CONF_START: 200, CONF_COUNT: 50},
]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HewalexGecoHub),
            cv.Required(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TARGET_ADDRESS, default=2): cv.uint8_t,
            cv.Optional(CONF_SOURCE_ADDRESS, default=1): cv.uint8_t,
            cv.Optional(CONF_FUNCTION_CODE, default=0x40): cv.hex_uint8_t,
            cv.Optional(
                CONF_REGISTER_RANGES, default=DEFAULT_RANGES
            ): cv.ensure_list(REGISTER_RANGE_SCHEMA),
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_FLOW_CONTROL_PIN])
    cg.add(var.set_flow_control_pin(pin))
    cg.add(var.set_target_address(config[CONF_TARGET_ADDRESS]))
    cg.add(var.set_source_address(config[CONF_SOURCE_ADDRESS]))
    cg.add(var.set_function_code(config[CONF_FUNCTION_CODE]))
    for r in config[CONF_REGISTER_RANGES]:
        cg.add(var.add_register_range(r[CONF_START], r[CONF_COUNT]))
