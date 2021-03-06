#!/bin/sh
CHECK_DEVICES="/dev/gps0"

# syntax systemdservice(:processname) (psname not needed if it equals servicename)
CHECK_SERVICES="ntpd gpsd asr:sr"

LOGLINES=10

output() {
    printf '=== System time: ===\n'
    date

    printf '\n=== Severe errors since last boot ===\n'
    journalctl --priority err --boot --lines=$LOGLINES

    printf '\n=== Checking devices ===\n'
    for dev in $CHECK_DEVICES; do
        printf '%s: ' "$dev"
        if [ -e "$dev" ]; then
            printf 'OK ('
            if [ -L "$dev" ]; then
                printf '%s -> ' "$dev"
            fi
            printf '%s)' "$(ls -la "$(readlink -f "$dev")")"
        else
            printf 'MISSING!'
        fi
        printf '\n'
    done

    printf '\n=== Checking system services ===\n'
    for serv in $CHECK_SERVICES; do
        service=$(printf '%s' "$serv" | cut -d ':' -f 1)
        psname=$(printf '%s' "$serv" | cut -d ':' -f 2)
        pid=$(pidof "$psname")
        printf '%s: ' "$service"
        if systemctl is-active "$service" --quiet; then
            if [ -n "$pid" ]; then
                printf 'OK (PID %s)' "$pid"
            else
                printf 'SERVICE ACTIVE BUT NO PROCESS!'
            fi
        else
            if [ -n "$pid" ]; then
                printf 'SERVICE INACTIVE BUT RUNNING WITH PID %s' "$pid"
            else
                printf 'SERVICE INACIVE AND NO PROCESS!'
            fi
        fi
        printf '\n'
    done

    SRPID=$(pidof sr)
    LOGS=""
    if [ -n "$SRPID" ]; then
        for pid in $SRPID; do
            LOGS="$(find /proc/$SRPID/fd -type l -exec ls -la {} \+ | grep log | sed 's/^.*-> //') $LOGS"
        done
        if [ -n "$LOGS" ]; then
            printf 'Possible log files: %s\n' "$LOGS"
        fi
    fi

    for log in "$LOGS"; do
        printf '=== %s last inits and errors/warnings in %s ===\n' "$LOGLINES" "$log"
        grep init < "$log" | tail -n $LOGLINES
        grep -e error -e warning < "$log" | tail -n $LOGLINES
    done
}

if [ "$1" = "monitor" ]; then
    while true; do
        clear
        output
        sleep 10
    done
else
    output
fi
