#include "ird_signal.h"
#include <string.h>

#define TAG "IrdSignal"

#define KEY_NAME       "name"
#define KEY_TYPE       "type"
#define KEY_PROTOCOL   "protocol"
#define KEY_ADDRESS    "address"
#define KEY_COMMAND    "command"
#define KEY_FREQUENCY  "frequency"
#define KEY_DUTY_CYCLE "duty_cycle"
#define KEY_DATA       "data"
#define TYPE_RAW       "raw"
#define TYPE_PARSED    "parsed"

IrdSignal* ird_signal_alloc(void) {
    IrdSignal* signal = malloc(sizeof(IrdSignal));
    memset(signal, 0, sizeof(IrdSignal));
    signal->message.protocol = InfraredProtocolUnknown;
    return signal;
}

static void ird_signal_clear_timings(IrdSignal* signal) {
    if(signal->timings) {
        free(signal->timings);
        signal->timings = NULL;
    }
    signal->timings_size = 0;
}

void ird_signal_free(IrdSignal* signal) {
    ird_signal_clear_timings(signal);
    free(signal);
}

static bool ird_message_is_valid(const InfraredMessage* message) {
    if(!infrared_is_protocol_valid(message->protocol)) return false;
    uint32_t address_length = infrared_get_protocol_address_length(message->protocol);
    uint32_t address_mask = (address_length >= 32) ? 0xFFFFFFFFUL : ((1UL << address_length) - 1);
    if(message->address != (message->address & address_mask)) return false;
    uint32_t command_length = infrared_get_protocol_command_length(message->protocol);
    uint32_t command_mask = (command_length >= 32) ? 0xFFFFFFFFUL : ((1UL << command_length) - 1);
    if(message->command != (message->command & command_mask)) return false;
    return true;
}

bool ird_signal_is_valid(const IrdSignal* signal) {
    if(signal->is_raw) {
        if(signal->timings == NULL || signal->timings_size == 0) return false;
        if(signal->timings_size > IRD_MAX_TIMINGS) return false;
        if(signal->frequency < INFRARED_MIN_FREQUENCY || signal->frequency > INFRARED_MAX_FREQUENCY)
            return false;
        if(signal->duty_cycle <= 0.0f || signal->duty_cycle > 1.0f) return false;
        return true;
    }
    return ird_message_is_valid(&signal->message);
}

void ird_signal_set_message(IrdSignal* signal, const InfraredMessage* message) {
    ird_signal_clear_timings(signal);
    signal->is_raw = false;
    signal->message = *message;
}

void ird_signal_set_raw(
    IrdSignal* signal,
    const uint32_t* timings,
    size_t timings_size,
    uint32_t frequency,
    float duty_cycle) {
    ird_signal_clear_timings(signal);
    signal->is_raw = true;
    signal->frequency = frequency;
    signal->duty_cycle = duty_cycle;
    if(timings_size > IRD_MAX_TIMINGS) timings_size = IRD_MAX_TIMINGS;
    signal->timings_size = timings_size;
    if(timings_size) {
        signal->timings = malloc(timings_size * sizeof(uint32_t));
        memcpy(signal->timings, timings, timings_size * sizeof(uint32_t));
    }
}

bool ird_signal_read_name(FlipperFormat* ff, FuriString* name) {
    return flipper_format_read_string(ff, KEY_NAME, name);
}

static bool ird_signal_read_message(IrdSignal* signal, FlipperFormat* ff) {
    bool ok = false;
    FuriString* buf = furi_string_alloc();
    InfraredMessage message = {0};
    do {
        if(!flipper_format_read_string(ff, KEY_PROTOCOL, buf)) break;
        message.protocol = infrared_get_protocol_by_name(furi_string_get_cstr(buf));
        if(!flipper_format_read_hex(ff, KEY_ADDRESS, (uint8_t*)&message.address, 4)) break;
        if(!flipper_format_read_hex(ff, KEY_COMMAND, (uint8_t*)&message.command, 4)) break;
        message.repeat = false;
        if(!ird_message_is_valid(&message)) {
            FURI_LOG_W(TAG, "Ungueltiges Signal (%s)", furi_string_get_cstr(buf));
            break;
        }
        ird_signal_set_message(signal, &message);
        ok = true;
    } while(false);
    furi_string_free(buf);
    return ok;
}

static bool ird_signal_read_raw(IrdSignal* signal, FlipperFormat* ff) {
    bool ok = false;
    uint32_t* timings = NULL;
    do {
        uint32_t frequency;
        float duty_cycle;
        uint32_t timings_size;
        if(!flipper_format_read_uint32(ff, KEY_FREQUENCY, &frequency, 1)) break;
        if(!flipper_format_read_float(ff, KEY_DUTY_CYCLE, &duty_cycle, 1)) break;
        if(!flipper_format_get_value_count(ff, KEY_DATA, &timings_size)) break;
        if(timings_size == 0 || timings_size > IRD_MAX_TIMINGS) {
            FURI_LOG_W(TAG, "RAW-Signal zu lang (%lu)", (unsigned long)timings_size);
            break;
        }
        timings = malloc(timings_size * sizeof(uint32_t));
        if(!flipper_format_read_uint32(ff, KEY_DATA, timings, timings_size)) break;
        ird_signal_set_raw(signal, timings, timings_size, frequency, duty_cycle);
        ok = true;
    } while(false);
    if(timings) free(timings);
    return ok;
}

bool ird_signal_read_body(IrdSignal* signal, FlipperFormat* ff) {
    bool ok = false;
    FuriString* type = furi_string_alloc();
    if(flipper_format_read_string(ff, KEY_TYPE, type)) {
        if(furi_string_equal(type, TYPE_RAW)) {
            ok = ird_signal_read_raw(signal, ff);
        } else if(furi_string_equal(type, TYPE_PARSED)) {
            ok = ird_signal_read_message(signal, ff);
        } else {
            FURI_LOG_W(TAG, "Unbekannter Signaltyp: %s", furi_string_get_cstr(type));
        }
    }
    furi_string_free(type);
    return ok;
}

bool ird_signal_search_by_index_and_read(IrdSignal* signal, FlipperFormat* ff, size_t index) {
    bool ok = false;
    FuriString* name = furi_string_alloc();
    for(size_t i = 0;; i++) {
        if(!ird_signal_read_name(ff, name)) break;
        if(i == index) {
            ok = ird_signal_read_body(signal, ff);
            break;
        }
    }
    furi_string_free(name);
    return ok;
}

bool ird_signal_save(const IrdSignal* signal, FlipperFormat* ff, const char* name) {
    if(!flipper_format_write_comment_cstr(ff, "")) return false;
    if(!flipper_format_write_string_cstr(ff, KEY_NAME, name)) return false;

    if(signal->is_raw) {
        if(!flipper_format_write_string_cstr(ff, KEY_TYPE, TYPE_RAW)) return false;
        if(!flipper_format_write_uint32(ff, KEY_FREQUENCY, &signal->frequency, 1)) return false;
        if(!flipper_format_write_float(ff, KEY_DUTY_CYCLE, &signal->duty_cycle, 1)) return false;
        if(!flipper_format_write_uint32(ff, KEY_DATA, signal->timings, signal->timings_size))
            return false;
    } else {
        const InfraredMessage* m = &signal->message;
        if(!flipper_format_write_string_cstr(ff, KEY_TYPE, TYPE_PARSED)) return false;
        if(!flipper_format_write_string_cstr(
               ff, KEY_PROTOCOL, infrared_get_protocol_name(m->protocol)))
            return false;
        if(!flipper_format_write_hex(ff, KEY_ADDRESS, (const uint8_t*)&m->address, 4)) return false;
        if(!flipper_format_write_hex(ff, KEY_COMMAND, (const uint8_t*)&m->command, 4)) return false;
    }
    return true;
}

void ird_signal_describe(const IrdSignal* signal, FuriString* out) {
    if(signal->is_raw) {
        furi_string_printf(
            out,
            "RAW, %u Flanken\n%lu Hz",
            (unsigned)signal->timings_size,
            (unsigned long)signal->frequency);
    } else {
        const InfraredMessage* m = &signal->message;
        furi_string_printf(
            out,
            "%s\nAdresse: 0x%0*lX\nBefehl: 0x%0*lX",
            infrared_get_protocol_name(m->protocol),
            (int)((infrared_get_protocol_address_length(m->protocol) + 3) / 4),
            (unsigned long)m->address,
            (int)((infrared_get_protocol_command_length(m->protocol) + 3) / 4),
            (unsigned long)m->command);
    }
}
