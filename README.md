axia-loudness-graphite-client
=============================

An <a href = "https://tech.ebu.ch/docs/events/ibc11-ebutechnical/presentations/ibc11_10things_r128.pdf">EBU R128</a> loudness meter for <a href = "http://www.axiaaudio.com/livewire">Axia Livewire</a>, an AoIP standard used in broadcast. The client sends EBU R128 shortterm loudness measurements once a second to your Graphite server via its <a href = "http://graphite.readthedocs.org/en/latest/feeding-carbon.html#the-plaintext-protocol">plaintext protocol</a>. Very useful for R128 logging, or building a loudness dashboard.

Correct usage: `axialufsgraphite <MC Livewire IP> <Graphite Server IP> <Graphite Metric Name>`

Example: `axialufsgraphite 239.192.2.169 127.0.0.1 Studio442PGM`
