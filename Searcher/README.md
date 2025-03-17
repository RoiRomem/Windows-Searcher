# Windows searcher

Windows searcher is a project inspired from the bloated "Power Toys Run" utility, written in the [C++](https://en.wikipedia.org/wiki/C%2B%2B) programming language

<hr/>

## Customization
To customize the search interface, you can edit the `style.cnf` file located in the build folder.

here's the default config:
```
win_edge_radius=15

font=Segoe UI

bk_r=20
bk_g=20
bk_b=30
bk_a=225

txt_r=255
txt_g=255
txt_b=255

sel_r=100
sel_g=100
sel_b=160

selo_r=120
selo_g=120
selo_b=200
```

#### Styling docs
please note that you do need all values to be set or the program won't work.


<hr />

`win_edge_radius` this is the value for the edge radius of the window, for example a `0` value would have blocky edges.

<hr />

`font` is the font that will be used in the search interface.

<hr />

`bk_r` red value for the background

`bk_g` green value for the background

`bk_b` blue value for the background

`bk_a` alpha(transparency) value for the background

<hr />

`txt_r` red value for the text in the search interface

`txt_g` green value for the text in the search interface

`txt_b` blue value for the text in the search interface

<hr />

`sel_r` red value for selected parts of the search interface

`sel_g` green value for selected parts of the search interface

`sel_b` blue value for selected parts of the search interface

<hr />

`selo_r` red value for the outline in the search interface

`selo_g` green value for the outline in the search interface

`selo_b` blue value for the outline in the search interface