# Task 269 diagnostic pair (rejected as visual proof)

| Capture | Cloud shadows | Other effective state | Size |
| --- | ---: | --- | ---: |
| [Off](cloud-off.jpg) | false in captured sidecar | sidecar claimed sun shadows=true, point lights=true, glow=false, ambient=1.000 | 514397 bytes |
| [On](cloud-on.jpg) | true in captured sidecar | sidecar claimed sun shadows=true, point lights=true, glow=false, ambient=1.000 | 515861 bytes |

Both 1920x1080 frames show the white-ground defect tracked by task 270. 877587 pixels (42.3%) are near-white in both frames. At an RGB channel threshold of 2, 250743 pixels differ, of which 6415 are in that white region. This difference does not establish a cloud-shadow change on visible ground. Treat this pair as rejected diagnostic evidence, not acceptance proof.
