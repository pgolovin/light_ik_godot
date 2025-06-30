#pragma once

#define DEFINE_PROPERTY(property_type, name) \
    void (set_##name)(const property_type& name); \
    property_type (get_##name)() const;

#define DECLARE_PROPERTY(class_name, name, property_type, prefix) \
    ClassDB::bind_method(D_METHOD(String("get_") + String(#name)), &class_name::get_##name); \
    ClassDB::bind_method(D_METHOD(String("set_") + String(#name)), &class_name::set_##name); \
    ADD_PROPERTY(PropertyInfo((property_type), String(#prefix) + "_" + String(#name)), (String("set_") + String(#name)), (String("get_") + String(#name)))

#define DECLARE_UNSCOPED_PROPERTY(class_name, name, property_type) \
    ClassDB::bind_method(D_METHOD(String("get_") + String(#name)), &class_name::get_##name); \
    ClassDB::bind_method(D_METHOD(String("set_") + String(#name)), &class_name::set_##name); \
    ADD_PROPERTY(PropertyInfo((property_type), String(#name)), (String("set_") + String(#name)), (String("get_") + String(#name)))
