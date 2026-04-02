/*
 * cookobj.cpp - Convert Tiled TMX to OTD/OMP binary
 * Uses: rapidxml (XML), toml.c (TOML)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include "rapidxml/rapidxml.hpp"

extern "C" {
#include "toml.h"
#include "psxtypes.h"
}

// Object IDs
enum ObjectId {
    OBJ_RING = 0x00, OBJ_MONITOR = 0x01, OBJ_SPIKES = 0x02,
    OBJ_CHECKPOINT = 0x03, OBJ_SPRING_YELLOW = 0x04, OBJ_SPRING_RED = 0x05,
    OBJ_SPRING_YELLOW_DIAGONAL = 0x06, OBJ_SPRING_RED_DIAGONAL = 0x07,
    OBJ_SWITCH = 0x08, OBJ_GOAL_SIGN = 0x09, OBJ_EXPLOSION = 0x0A,
    OBJ_MONITOR_IMAGE = 0x0B, OBJ_SHIELD = 0x0C, OBJ_BUBBLE_PATCH = 0x0D,
    OBJ_BUBBLE = 0x0E, OBJ_END_CAPSULE = 0x0F, OBJ_END_CAPSULE_BUTTON = 0x10,
    OBJ_DOOR = 0x11, OBJ_ANIMAL = 0x12, OBJ_AMY_HEART = 0x13
};

enum DummyObjectId { DUMMY_RING_3H = -1, DUMMY_RING_3V = -2, DUMMY_STARTPOS = -3 };

enum MonitorKind { MON_NONE=0, MON_RING=1, MON_SPEEDSHOES=2, MON_SHIELD=3,
                   MON_INVINCIBILITY=4, MON_LIFE=5, MON_SUPER=6 };

// Frame
typedef struct {
    uint8_t u0, v0, width, height, flipmask, tpage;
} Frame;

// Animation
typedef struct {
    std::vector<Frame> frames;
    int8_t loopback;
    uint8_t duration;
} Animation;

// Object Fragment
typedef struct {
    int16_t offsetx, offsety;
    std::vector<Animation> animations;
} Fragment;

// Object Definition
typedef struct {
    int8_t id;
    std::string name;
    std::vector<Animation> animations;
    Fragment* fragment;
} ObjectDef;

// Object Map (OTD)
typedef struct {
    uint8_t is_level_specific;
    uint16_t num_objs;
    std::map<int, ObjectDef> objects;
    std::map<int, int> obj_mapping;
} ObjectMap;

// Object Placement
typedef struct {
    uint8_t is_level_specific;
    int8_t otype;
    uint16_t unique_id, parent_id;
    uint8_t flipmask;
    int32_t x, y;
    uint8_t has_props;
    uint8_t prop_kind;
} Placement;

int get_obj_id(const char* name) {
    if (!name) return -1;
    if (strcasecmp(name, "ring") == 0) return OBJ_RING;
    if (strcasecmp(name, "monitor") == 0) return OBJ_MONITOR;
    if (strcasecmp(name, "spikes") == 0) return OBJ_SPIKES;
    if (strcasecmp(name, "checkpoint") == 0) return OBJ_CHECKPOINT;
    if (strcasecmp(name, "spring_yellow") == 0) return OBJ_SPRING_YELLOW;
    if (strcasecmp(name, "spring_red") == 0) return OBJ_SPRING_RED;
    if (strcasecmp(name, "spring_yellow_diagonal") == 0) return OBJ_SPRING_YELLOW_DIAGONAL;
    if (strcasecmp(name, "spring_red_diagonal") == 0) return OBJ_SPRING_RED_DIAGONAL;
    if (strcasecmp(name, "switch") == 0) return OBJ_SWITCH;
    if (strcasecmp(name, "goal_sign") == 0) return OBJ_GOAL_SIGN;
    if (strcasecmp(name, "explosion") == 0) return OBJ_EXPLOSION;
    if (strcasecmp(name, "monitor_image") == 0) return OBJ_MONITOR_IMAGE;
    if (strcasecmp(name, "shield") == 0) return OBJ_SHIELD;
    if (strcasecmp(name, "bubble_patch") == 0) return OBJ_BUBBLE_PATCH;
    if (strcasecmp(name, "bubble") == 0) return OBJ_BUBBLE;
    if (strcasecmp(name, "end_capsule") == 0) return OBJ_END_CAPSULE;
    if (strcasecmp(name, "end_capsule_button") == 0) return OBJ_END_CAPSULE_BUTTON;
    if (strcasecmp(name, "door") == 0) return OBJ_DOOR;
    if (strcasecmp(name, "animal") == 0) return OBJ_ANIMAL;
    if (strcasecmp(name, "amy_heart") == 0) return OBJ_AMY_HEART;
    return -1;
}

int get_dummy_id(const char* name) {
    if (!name) return 0;
    if (strcasecmp(name, "ring_3h") == 0) return DUMMY_RING_3H;
    if (strcasecmp(name, "ring_3v") == 0) return DUMMY_RING_3V;
    if (strcasecmp(name, "startpos") == 0) return DUMMY_STARTPOS;
    return 0;
}

MonitorKind get_monitor_kind(const char* name) {
    if (!name) return MON_NONE;
    if (strcasecmp(name, "RING") == 0) return MON_RING;
    if (strcasecmp(name, "SPEEDSHOES") == 0) return MON_SPEEDSHOES;
    if (strcasecmp(name, "SHIELD") == 0) return MON_SHIELD;
    if (strcasecmp(name, "INVINCIBILITY") == 0) return MON_INVINCIBILITY;
    if (strcasecmp(name, "1UP") == 0) return MON_LIFE;
    if (strcasecmp(name, "SUPER") == 0) return MON_SUPER;
    return MON_NONE;
}

void write_frame(FILE* f, const Frame& fr) {
    write_u8(f, fr.u0);
    write_u8(f, fr.v0);
    write_u8(f, fr.width);
    write_u8(f, fr.height);
    write_u8(f, fr.flipmask);
    write_u8(f, fr.tpage);
}

void write_animation(FILE* f, const Animation& anim) {
    write_u16_be(f, anim.frames.size());
    write_s8(f, anim.loopback);
    write_u8(f, anim.duration);
    for (const auto& fr : anim.frames) {
        write_frame(f, fr);
    }
}

void write_fragment(FILE* f, const Fragment* frag) {
    if (!frag) return;
    write_s16_be(f, frag->offsetx);
    write_s16_be(f, frag->offsety);
    write_u16_be(f, frag->animations.size());
    for (const auto& anim : frag->animations) {
        write_animation(f, anim);
    }
}

void write_otd(const char* filename, ObjectMap& map) {
    FILE* f = fopen(filename, "wb");
    if (!f) { std::cerr << "Cannot create " << filename << std::endl; return; }
    
    write_u8(f, map.is_level_specific);
    write_u16_be(f, map.num_objs);
    
    for (auto& [gid, obj] : map.objects) {
        write_s8(f, obj.id);
        write_u8(f, obj.fragment ? 1 : 0);
        write_u16_be(f, obj.animations.size());
        for (const auto& anim : obj.animations) {
            write_animation(f, anim);
        }
        if (obj.fragment) {
            write_fragment(f, obj.fragment);
        }
    }
    
    fclose(f);
    std::cout << "Created " << filename << " (" << map.num_objs << " objects)" << std::endl;
}

void write_omp(const char* filename, std::vector<Placement>& placements) {
    FILE* f = fopen(filename, "wb");
    if (!f) { std::cerr << "Cannot create " << filename << std::endl; return; }
    
    write_u16_be(f, placements.size());
    
    for (const auto& p : placements) {
        write_u8(f, p.is_level_specific);
        write_s8(f, p.otype);
        write_u16_be(f, p.unique_id);
        write_u16_be(f, p.parent_id);
        write_u8(f, p.flipmask);
        write_s32_be(f, p.x);
        write_s32_be(f, p.y);
        if (p.has_props) {
            write_u8(f, p.prop_kind);
        }
    }
    
    fclose(f);
    std::cout << "Created " << filename << " (" << placements.size() << " placements)" << std::endl;
}

void parse_toml_animations(const char* filename, ObjectDef& obj) {
    FILE* f = fopen(filename, "r");
    if (!f) return;
    
    char errbuf[256];
    toml_table_t* root = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);
    if (!root) return;
    
    toml_array_t* anims = toml_array_in(root, "animations");
    if (anims) {
        for (int i = 0; ; i++) {
            toml_table_t* anim_tab = toml_table_at(anims, i);
            if (!anim_tab) break;
            
            Animation anim;
            toml_datum_t loop = toml_int_in(anim_tab, "loopback");
            toml_datum_t dur = toml_int_in(anim_tab, "duration");
            anim.loopback = loop.ok ? loop.u.i : 0;
            anim.duration = dur.ok ? dur.u.i : 0;
            
            toml_array_t* frames = toml_array_in(anim_tab, "frames");
            if (frames) {
                for (int j = 0; ; j++) {
                    toml_array_t* frame_arr = toml_array_at(frames, j);
                    if (!frame_arr) break;
                    Frame fr = {};
                    toml_datum_t d0 = toml_int_at(frame_arr, 0);
                    toml_datum_t d1 = toml_int_at(frame_arr, 1);
                    toml_datum_t d2 = toml_int_at(frame_arr, 2);
                    toml_datum_t d3 = toml_int_at(frame_arr, 3);
                    if (d0.ok) fr.u0 = d0.u.i;
                    if (d1.ok) fr.v0 = d1.u.i;
                    if (d2.ok) fr.width = d2.u.i;
                    if (d3.ok) fr.height = d3.u.i;
                    fr.tpage = fr.v0 / 256;
                    fr.v0 %= 256;
                    anim.frames.push_back(fr);
                }
            }
            obj.animations.push_back(anim);
        }
    }
    
    toml_table_t* frag = toml_table_in(root, "fragment");
    if (frag) {
        obj.fragment = new Fragment();
        toml_datum_t offx = toml_int_in(frag, "offsetx");
        toml_datum_t offy = toml_int_in(frag, "offsety");
        if (offx.ok) obj.fragment->offsetx = offx.u.i;
        if (offy.ok) obj.fragment->offsety = offy.u.i;
        
        toml_array_t* fanims = toml_array_in(frag, "animations");
        if (fanims) {
            for (int i = 0; ; i++) {
                toml_table_t* anim_tab = toml_table_at(fanims, i);
                if (!anim_tab) break;
                Animation anim;
                toml_datum_t loop = toml_int_in(anim_tab, "loopback");
                toml_datum_t dur = toml_int_in(anim_tab, "duration");
                anim.loopback = loop.ok ? loop.u.i : 0;
                anim.duration = dur.ok ? dur.u.i : 0;
                obj.fragment->animations.push_back(anim);
            }
        }
    }
    
    toml_free(root);
}

void parse_tileset(const char* tsx_path, int firstgid, ObjectMap& map) {
    std::ifstream file(tsx_path);
    if (!file.is_open()) return;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xml = buffer.str();
    file.close();
    
    rapidxml::xml_document<> doc;
    char* cstr = new char[xml.size() + 1];
    strcpy(cstr, xml.c_str());
    doc.parse<0>(cstr);
    
    rapidxml::xml_node<>* tileset = doc.first_node("tileset");
    if (!tileset) { delete[] cstr; return; }
    
    const char* name = tileset->first_attribute("name") ? tileset->first_attribute("name")->value() : "";
    map.is_level_specific = (strcmp(name, "objects_common") != 0);
    
    char toml_path[1024];
    strcpy(toml_path, tsx_path);
    char* dot = strrchr(toml_path, '.');
    if (dot) strcpy(dot, ".toml");
    
    int obj_id = 0;
    for (rapidxml::xml_node<>* tile = tileset->first_node("tile"); tile; tile = tile->next_sibling("tile")) {
        rapidxml::xml_attribute<>* id_attr = tile->first_attribute("id");
        rapidxml::xml_attribute<>* type_attr = tile->first_attribute("type");
        if (!id_attr) continue;
        
        int tid = atoi(id_attr->value());
        int gid = tid + firstgid;
        const char* type = type_attr ? type_attr->value() : "";
        
        int dummy_id = get_dummy_id(type);
        if (dummy_id) {
            map.obj_mapping[gid] = dummy_id;
            continue;
        }
        
        int obj_type = get_obj_id(type);
        if (obj_type < 0) continue;
        
        ObjectDef obj;
        obj.id = obj_id++;
        obj.name = type;
        obj.fragment = nullptr;
        
        parse_toml_animations(toml_path, obj);
        
        map.objects[gid] = obj;
        map.obj_mapping[gid] = map.is_level_specific ? (gid - firstgid) : obj_type;
    }
    
    map.num_objs = map.objects.size();
    delete[] cstr;
}

void parse_tmx(const char* tmx_path, std::map<std::string, ObjectMap>& maps, std::vector<Placement>& placements) {
    std::ifstream file(tmx_path);
    if (!file.is_open()) return;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xml = buffer.str();
    file.close();
    
    rapidxml::xml_document<> doc;
    char* cstr = new char[xml.size() + 1];
    strcpy(cstr, xml.c_str());
    doc.parse<0>(cstr);
    
    rapidxml::xml_node<>* map = doc.first_node("map");
    if (!map) { delete[] cstr; return; }
    
    rapidxml::xml_node<>* objects_group = nullptr;
    for (rapidxml::xml_node<>* g = map->first_node("group"); g; g = g->next_sibling("group")) {
        rapidxml::xml_attribute<>* name = g->first_attribute("name");
        if (name && strcmp(name->value(), "OBJECTS") == 0) {
            objects_group = g;
            break;
        }
    }
    
    if (!objects_group) { delete[] cstr; return; }
    
    for (rapidxml::xml_node<>* og = objects_group->first_node("objectgroup"); og; og = og->next_sibling("objectgroup")) {
        for (rapidxml::xml_node<>* obj = og->first_node("object"); obj; obj = obj->next_sibling("object")) {
            rapidxml::xml_attribute<>* gid_attr = obj->first_attribute("gid");
            if (!gid_attr) continue;
            
            int gid = atoi(gid_attr->value());
            int type_id = -1;
            uint8_t is_level_specific = 0;
            
            for (auto& [name, map] : maps) {
                auto it = map.obj_mapping.find(gid & ~(0x7u << 29));
                if (it != map.obj_mapping.end()) {
                    type_id = it->second;
                    is_level_specific = map.is_level_specific;
                    break;
                }
            }
            
            if (type_id == -1) continue;
            
            Placement p;
            p.is_level_specific = is_level_specific;
            p.otype = type_id;
            p.unique_id = atoi(obj->first_attribute("id") ? obj->first_attribute("id")->value() : "0");
            p.parent_id = 0;
            p.x = atoi(obj->first_attribute("x")->value());
            p.y = atoi(obj->first_attribute("y")->value());
            
            p.flipmask = 0;
            if (gid & (1 << 31)) p.flipmask |= 1;
            if (gid & (1 << 30)) p.flipmask |= 2;
            
            rapidxml::xml_attribute<>* rot = obj->first_attribute("rotation");
            if (rot) {
                int r = atoi(rot->value());
                if (r == 90) p.flipmask |= 4;
                if (r == -90) p.flipmask |= 8;
            }
            
            p.has_props = 0;
            p.prop_kind = 0;
            
            rapidxml::xml_node<>* props = obj->first_node("properties");
            if (props) {
                for (rapidxml::xml_node<>* prop = props->first_node("property"); prop; prop = prop->next_sibling("property")) {
                    const char* pname = prop->first_attribute("name")->value();
                    const char* pval = prop->first_attribute("value") ? prop->first_attribute("value")->value() : "";
                    
                    if (strcmp(pname, "Kind") == 0 && type_id == OBJ_MONITOR) {
                        p.has_props = 1;
                        p.prop_kind = get_monitor_kind(pval);
                    }
                    if (strcmp(pname, "frequency") == 0 && type_id == OBJ_BUBBLE_PATCH) {
                        p.has_props = 1;
                        p.prop_kind = atoi(pval);
                    }
                    if (strcmp(pname, "parent") == 0) {
                        p.parent_id = atoi(pval);
                    }
                }
            }
            
            placements.push_back(p);
        }
    }
    
    delete[] cstr;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input.tmx>" << std::endl;
        return 1;
    }
    
    char base_path[1024];
    strcpy(base_path, argv[1]);
    char* last_slash = strrchr(base_path, '/');
    if (last_slash) *last_slash = '\0';
    else { base_path[0] = '.'; base_path[1] = '\0'; }
    
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open " << argv[1] << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xml = buffer.str();
    file.close();
    
    rapidxml::xml_document<> doc;
    char* cstr = new char[xml.size() + 1];
    strcpy(cstr, xml.c_str());
    doc.parse<0>(cstr);
    
    rapidxml::xml_node<>* map = doc.first_node("map");
    if (!map) { delete[] cstr; return 1; }
    
    std::map<std::string, ObjectMap> obj_maps;
    
    for (rapidxml::xml_node<>* ts = map->first_node("tileset"); ts; ts = ts->next_sibling("tileset")) {
        rapidxml::xml_attribute<>* src = ts->first_attribute("source");
        rapidxml::xml_attribute<>* fg = ts->first_attribute("firstgid");
        if (!src || !fg) continue;
        
        if (strstr(src->value(), "128")) continue;
        
        char tsx_path[1024];
        snprintf(tsx_path, sizeof(tsx_path), "%s/%s", base_path, src->value());
        
        int firstgid = atoi(fg->value());
        ObjectMap omap;
        parse_tileset(tsx_path, firstgid, omap);
        
        const char* name = strrchr(tsx_path, '/');
        if (!name) name = tsx_path;
        else name++;
        const char* dot = strchr(name, '.');
        if (dot) {
            char n[256];
            strncpy(n, name, dot - name);
            n[dot - name] = '\0';
            obj_maps[n] = omap;
        }
        
        char otd_path[1024];
        strcpy(otd_path, tsx_path);
        char* dot_tsx = strrchr(otd_path, '.');
        if (dot_tsx) strcpy(dot_tsx, ".OTD");
        write_otd(otd_path, omap);
    }
    
    delete[] cstr;
    
    std::vector<Placement> placements;
    parse_tmx(argv[1], obj_maps, placements);
    
    char omp_path[1024];
    strcpy(omp_path, argv[1]);
    char* dot = strrchr(omp_path, '.');
    if (dot) strcpy(dot, ".OMP");
    else strcat(omp_path, ".OMP");
    write_omp(omp_path, placements);
    
    return 0;
}
