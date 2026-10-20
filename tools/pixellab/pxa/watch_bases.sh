#!/bin/bash
# Prints "fighter/clip status" once for every base clip that finished (done/failed/rejected) and is not approved yet.
cd "$(dirname "$0")"
seen=" $(tr '\n' ' ' < approved.txt) "
while true; do
  for f in asta yuno ichigo byakuya kenpachi ulquiorra grimmjow starrk gojo sukuna link ganondorf kratos hunter; do
    for c in jump runstart knockdown holdstart ledgegrab w0guard w1guard; do
      k="$f/$c"
      case "$seen" in *" $k "*) continue;; esac
      [ -f $k/job.json ] || continue
      st=$(python3 -c "import json;print(json.load(open('$k/job.json')).get('status'))" 2>/dev/null)
      case "$st" in done|failed|rejected|error) echo "$k $st"; seen="$seen$k ";; esac
    done
  done
  sleep 60
done
