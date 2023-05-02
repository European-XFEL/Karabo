**************
Stateful Icons
**************

Stateful icons are icons which are automatically colored depending on the
state of the device they represent. The graphics forming the base of each
icon are expected to be provided in SVG format, which needs to meet the
following requirements:

- The icon is contained entirely in a SVG group tag (`svg:g`) which has an
  `id` attribute beginning with `icon_`.

- Within this SVG group a description tag `desc` needs to exist, that gives
  a short description of the icon. This description is the entry shown in the
  selection context menu. Short thus means max. 3 words.

- Within this group and SVG elements with the fill-color white (`#FFFFFF`)
  will be recolored to the respective state color. The stroke color stays
  untouched.

- All SVG elements need to be prefixed by an `svg:` namespace.

- The group elements transform must be relative to a (0,0) origin.

Creating Stateful Icons with Inkscape
-------------------------------------

Inkscape is a popular open source SVG editor. To meet the above requirements
it needs to be configured appropriately and some specific steps need to be
taken:

- Configure Inkscape to make less asumptions: go to
  *File->Inkscape Preferences* and then *Transforms*. The selection in
  *Store tranformation* should be set to *preserved*. In *SVG output* under
  *Path data* deselect *Allow relative coordinates* and select *Force repeat
  commands*.

- Create your icon and make sure that all areas that should be filled with
  the state-specific color have their fill color set to plain white (`#FFFFFF`).
  Group your item.

- Set the document size to match your icon size: Click on
  *File->Document properties* and then select *Resize page to drawing...*

- Edit the group attributes. Click on your group and open the context menu
  (right-click). Click on *Object properities*. Enter an *id* starting with
  *icon_* and a short description. Dont forget to click *Set*.

- Make sure the tranform of your group is relative to an origin of (0,0).
  Open the *tranform* dialog: *Object->Tranform*. Make sure that
  *relative move* is not checked and *Apply to each object separately* is
  checked. Now open the XML editor: *Edit->XML Editor*. If you see an outer
  group with a *Layer* id around your icon group, click on this and delete
  its transform attribute. Your icon will now move out of the drawing area.
  Move the icon to a (0,0) origin, by clicking on it, and in the *Tranform*
  dialog setting *Horizontal* and *Vertical Move* to (0,0) and then clicking
  *Apply*.

- Finally, save your icon as a plain SVG file.
