<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.11.0" name="objects_common" tilewidth="64" tileheight="64" tilecount="16" columns="4">
 <image source="obj_common_designtiles.png" width="256" height="256"/>
 <tile id="0" type="ring"/>
 <tile id="1" type="monitor">
  <properties>
   <property name="Kind" value="NONE"/>
  </properties>
  <objectgroup draworder="index" id="2">
   <object id="1" x="16" y="34" width="30" height="30"/>
  </objectgroup>
 </tile>
 <tile id="2" type="spikes">
  <objectgroup draworder="index" id="2">
   <object id="1" x="16" y="32" width="32" height="32"/>
  </objectgroup>
 </tile>
 <tile id="3" type="checkpoint"/>
 <tile id="4" type="spring_yellow">
  <objectgroup draworder="index" id="2">
   <object id="1" x="16" y="48" width="32" height="16"/>
  </objectgroup>
 </tile>
 <tile id="5" type="spring_red">
  <objectgroup draworder="index" id="2">
   <object id="1" x="16" y="48" width="32" height="16"/>
  </objectgroup>
 </tile>
 <tile id="6" type="spring_yellow_diagonal">
  <objectgroup draworder="index" id="2">
   <object id="2" x="17" y="32">
    <polygon points="0,0 9.69078,0 31.641,22.1837 31.641,32.2248 -0.583782,32.108"/>
   </object>
  </objectgroup>
 </tile>
 <tile id="7" type="spring_red_diagonal">
  <objectgroup draworder="index" id="2">
   <object id="1" x="17" y="32">
    <polygon points="0,0 9.69078,0 31.641,22.1837 31.641,32.2248 -0.583782,32.108"/>
   </object>
  </objectgroup>
 </tile>
 <tile id="8" type="ring_3h"/>
 <tile id="9" type="switch">
  <objectgroup draworder="index" id="2">
   <object id="1" x="16" y="48" width="32" height="16"/>
  </objectgroup>
 </tile>
 <tile id="10" type="ring_3v"/>
 <tile id="11" type="goal_sign"/>
 <tile id="12" type="startpos"/>
</tileset>
