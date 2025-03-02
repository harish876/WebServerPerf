Profile
```
sudo profile-bpfcc -f -F 99 -C 0 -p $(pgrep -f http_server_rust | sed -n '2p') > out_rust.profile-folded
```

Generate Flamegraph
```
```

Turns out when i run pid using taskset bcc is not genrating call stack