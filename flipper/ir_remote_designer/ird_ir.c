#include "ird_app_i.h"
#include <furi_hal_infrared.h>

#define TAG "IrdIr"
#define IRD_TX_MIN_INTERVAL_MS 50U

bool ird_tx_start(IrdApp* app, uint8_t button) {
    if(app->is_transmitting) return true;
    if(app->is_receiving) return false;

    const uint32_t elapsed = furi_get_tick() - app->last_tx_tick;
    if(elapsed < IRD_TX_MIN_INTERVAL_MS) return false;

    if(!ird_remote_load_signal(app->remote, app->storage, button, app->tx_signal)) {
        FURI_LOG_W(TAG, "Kein Signal fuer Knopf %u", button);
        return false;
    }
    if(furi_hal_infrared_is_busy()) return false;

    if(app->tx_signal->is_raw) {
        const IrdSignal* raw = app->tx_signal;
        infrared_worker_set_raw_signal(
            app->worker, raw->timings, raw->timings_size, raw->frequency, raw->duty_cycle);
    } else {
        infrared_worker_set_decoded_signal(app->worker, &app->tx_signal->message);
    }

    notification_message(app->notifications, &sequence_blink_start_magenta);
    infrared_worker_tx_set_get_signal_callback(
        app->worker, infrared_worker_tx_get_signal_steady_callback, app);
    infrared_worker_tx_start(app->worker);
    app->is_transmitting = true;
    ird_remote_view_set_transmitting(app->remote_view, true);
    return true;
}

void ird_tx_stop(IrdApp* app) {
    if(!app->is_transmitting) return;
    infrared_worker_tx_stop(app->worker);
    infrared_worker_tx_set_get_signal_callback(app->worker, NULL, NULL);
    notification_message(app->notifications, &sequence_blink_stop);
    app->is_transmitting = false;
    app->last_tx_tick = furi_get_tick();
    ird_remote_view_set_transmitting(app->remote_view, false);
}

static void ird_rx_callback(void* context, InfraredWorkerSignal* received_signal) {
    IrdApp* app = context;
    if(infrared_worker_signal_is_decoded(received_signal)) {
        ird_signal_set_message(
            app->pending_signal, infrared_worker_get_decoded_signal(received_signal));
    } else {
        const uint32_t* timings;
        size_t timings_size;
        infrared_worker_get_raw_signal(received_signal, &timings, &timings_size);
        ird_signal_set_raw(
            app->pending_signal,
            timings,
            timings_size,
            INFRARED_COMMON_CARRIER_FREQUENCY,
            INFRARED_COMMON_DUTY_CYCLE);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, IrdCustomEventSignalReceived);
}

void ird_rx_start(IrdApp* app) {
    if(app->is_receiving) return;
    ird_tx_stop(app);
    infrared_worker_rx_set_received_signal_callback(app->worker, ird_rx_callback, app);
    infrared_worker_rx_enable_blink_on_receiving(app->worker, true);
    infrared_worker_rx_start(app->worker);
    app->is_receiving = true;
}

void ird_rx_stop(IrdApp* app) {
    if(!app->is_receiving) return;
    infrared_worker_rx_stop(app->worker);
    infrared_worker_rx_set_received_signal_callback(app->worker, NULL, NULL);
    app->is_receiving = false;
}
