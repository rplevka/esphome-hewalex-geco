import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from .. import (
    CONF_HEWALEX_GECO_ID,
    HewalexGecoHub,
    hewalex_geco_ns,
)

DEPENDENCIES = ["hewalex_geco"]

HewalexGecoSensor = hewalex_geco_ns.class_(
    "HewalexGecoSensor", sensor.Sensor, cg.Component
)

DecodeType = hewalex_geco_ns.enum("DecodeType")
DECODE_TYPES = {
    "unsigned": DecodeType.DECODE_U16,
    "signed": DecodeType.DECODE_S16,
}

CONF_REGISTER = "register"
CONF_DECODE = "decode"
CONF_DIVISOR = "divisor"

CONFIG_SCHEMA = (
    sensor.sensor_schema(HewalexGecoSensor)
    .extend(
        {
            cv.GenerateID(CONF_HEWALEX_GECO_ID): cv.use_id(HewalexGecoHub),
            cv.Required(CONF_REGISTER): cv.uint16_t,
            cv.Optional(CONF_DECODE, default="unsigned"): cv.enum(
                DECODE_TYPES, lower=True
            ),
            cv.Optional(CONF_DIVISOR, default=1.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_HEWALEX_GECO_ID])
    cg.add(var.set_register(config[CONF_REGISTER]))
    cg.add(var.set_decode(config[CONF_DECODE]))
    cg.add(var.set_divisor(config[CONF_DIVISOR]))
    cg.add(hub.add_sensor(var))
