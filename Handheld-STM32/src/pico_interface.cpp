#include "pico_interface.h"

PicoInterface::PicoInterface(uint32_t Tx, uint32_t Rx)
{
    memset(&pBuf,0,sizeof(pBuf));
    memset(&pDiag,0,sizeof(pDiag));
    ser = new HardwareSerial(Rx, Tx);
    ser->begin(PICO_SERIAL_BAUD);
}

void PicoInterface::wait_until_ready()
{
    while (ser->read() != 0x06)
    {
    };
}

void PicoInterface::update_can_state(bool canOnline)
{
    if (canOnline)
    {
        pBuf.flags |= 1 << PICO_PACK_FLAG_CAN_STAT;
    }
    else
    {
        pBuf.flags &= ~(1 << PICO_PACK_FLAG_CAN_STAT);
    }
}

void PicoInterface::update_dsd_state(bool dsdActive)
{
    if (dsdActive)
    {
        pBuf.flags |= 1 << PICO_PACK_FLAG_DSD_STAT;
        pDiag.flags |= 1 << PICO_DIAG_FLAG_DSD;
    }
    else
    {
        pBuf.flags &= ~(1 << PICO_PACK_FLAG_DSD_STAT);
        pDiag.flags &= ~(1 << PICO_DIAG_FLAG_DSD);
    }
}

void PicoInterface::import_states(incoming_vars *ivar)
{
    pBuf.bus12 = ivar->bus12v;
    pBuf.bus24 = ivar->bus24v;
    pBuf.bus36 = ivar->bus36v;
    pBuf.buscap = ivar->cap_volts;
    pBuf.busTrac = ivar->bus_vesc;
    pBuf.message = ivar->msg_push;
    pBuf.regenEnergy = ivar->super_cap_energy;
    pBuf.tractionCurrent = ivar->trac_current;
    uint32_t flags_temp = 0 | (ivar->direction_flag & 1) << PICO_PACK_FLAG_DIR | (ivar->kill_switch_relay & 1) << PICO_PACK_FLAG_KSR | (ivar->h_e_stop & 1) << PICO_PACK_FLAG_ES | (ivar->s_e_stop & 1) << PICO_PACK_FLAG_SOFT_ES | (ivar->brake_1 & 1) << PICO_PACK_FLAG_BK_BR1 | (ivar->brake_2 & 1) << PICO_PACK_FLAG_BK_BR2 | (ivar->brake_3 & 1) << PICO_PACK_FLAG_BK_BR3 | (ivar->master_brake_relay & 1) << PICO_PACK_FLAG_BK_MBR | (ivar->brake_fault & 1) << PICO_PACK_FLAG_BF | ((ivar->drive_mode == drive_mode_t::Autostop) & 1) << PICO_PACK_FLAG_AS_EN | (ivar->stationary_lock & 1) << PICO_PACK_FLAG_BK_BH | ((ivar->drive_mode == drive_mode_t::Regen) & 1) << PICO_PACK_FLAG_RG_EN | (ivar->autostop_marker_found & 1) << PICO_PACK_FLAG_AS_MD | (ivar->fire_alarm & 1) << PICO_PACK_FLAG_FIRE | (ivar->auto_throttle_en & 1) << PICO_PACK_FLAG_ATE | (ivar->driver_throttle_en & 1) << PICO_PACK_FLAG_DTE | ((ivar->drive_mode == drive_mode_t::Shunting) & 1) << PICO_PACK_FLAG_SHUNT_MODE | ((ivar->coupler_status) & 3) << PICO_PACK_COUPLER;
    // Clear the bits for the flags we are writing
    pBuf.flags &= ~(1 << PICO_PACK_FLAG_DIR | 1 << PICO_PACK_FLAG_KSR | 1 << PICO_PACK_FLAG_ES | 1 << PICO_PACK_FLAG_BK_BR1 | 1 << PICO_PACK_FLAG_BK_BR2 | 1 << PICO_PACK_FLAG_BK_BR3 | 1 << PICO_PACK_FLAG_BK_MBR | 1 << PICO_PACK_FLAG_BF | 1 << PICO_PACK_FLAG_AS_EN | 1 << PICO_PACK_FLAG_BK_BH | 1 << PICO_PACK_FLAG_RG_EN | 1 << PICO_PACK_FLAG_AS_MD | 1 << PICO_PACK_FLAG_FIRE | 1 << PICO_PACK_FLAG_SOFT_ES | 1 << PICO_PACK_FLAG_ATE | 1 << PICO_PACK_FLAG_DTE | 1 << PICO_PACK_FLAG_SHUNT_MODE | 3 << PICO_PACK_COUPLER);
    // Write our flags with OR.
    pBuf.flags |= flags_temp;
}

void PicoInterface::import_vals(tracked_outgoing *cvar)
{
    pDiag.flags = 0 | (cvar->local_brake_demand) | (cvar->autostop_btn & 1) << PICO_DIAG_FLAG_AS | (cvar->couple & 1) << PICO_DIAG_FLAG_AC | (cvar->change_direction & 1) << PICO_DIAG_FLAG_CD | (cvar->horn& 1) << PICO_DIAG_FLAG_HR | (cvar->regen_btn & 1) << PICO_DIAG_FLAG_RG;
}

void PicoInterface::import_analogue_values(uint16_t brake, uint16_t throttle)
{
    pDiag.brakeAnalogue = brake;
    pDiag.throttleAnalogue = throttle;
}

void PicoInterface::tx_pico_pack()
{
    uint8_t buf[PICO_PACK_ENCODED_PAYLOAD_SIZE];
    encode_base64_cl((unsigned char *)&pBuf, sizeof(pBuf), buf);
    uint8_t csum = 0x75; // Precalced sum of 'D1'
    ser->print(PICO_PACK_PREFIX);
    for (int i = 0; i < sizeof(buf); i++)
    {
        ser->write(buf[i]);
        //Serial.write(buf[i]);
        csum+=buf[i];
    }
    // If the checksum would be 0x0a or 0x0d, add stuffing bytes.
    if(csum == 0xA0 || csum == 0x9D){
        csum+= 0x01;
        ser->write(0x01);
    }
    csum = 0xAA-csum;
    ser->write(csum);
    ser->write('\n');
    //Serial.write(csum);
    //Serial.write('\n');
    ser->flush();
}

void PicoInterface::tx_pico_diag()
{
    pDiag.rand++;
    uint8_t buf[PICO_DIAG_ENCODED_PAYLOAD_SIZE];
    encode_base64_cl((unsigned char *)&pDiag, sizeof(pDiag), buf);
    uint8_t csum = 0xBC; // Precalced sum of 'Dx'
    ser->print(PICO_DIAG_PREFIX);
    for (int i = 0; i < sizeof(buf); i++)
    {
        ser->write(buf[i]);
        csum+=buf[i];
    }
    if (csum == 0xA0 || csum == 0x9D)
    {
        csum += 0x01;
        ser->write(0x01);
    }
    csum = 0xAA-csum;
    ser->write(csum);
    ser->write('\n');
    ser->flush();
}
