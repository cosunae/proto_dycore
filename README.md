
<div style="text-align:center"><img src="icon.png" width="500" height="200" border="0" alt="proto dycore" /></div>


Prototype design of a field management layer for a dycore using GridTools. 
The design contains several concepts: 
 
 * a `context`: that describes the list of fields that belong and are allocated/used within this context. 
Examples of a `context` are, a vertical advection operator, a fast waves framework, a time integrator or a full dycore. 
Contexts are inclusive, i.e. when a context is activated, all fields belonging to already activated `contexts` will be inhereted. 
Like that a fast waves context is included in a dycore `context`, meaning that the fast waves can access any field declared in the dycore
context without having to declare it for its specific `context`
 * a `repository`: stores and manages all fields associated to a `context`. Fields are stored in tuples, and the repository can handle all types of storages, i.e. 3d fields, 2d fields, etc. 
 * a `field pool`: emulates a memory pool, by giving access to the user *only* to fields of active contexts. The `field pool` contains all the repositories defined in the dycore (one per each `context`)
and additionally handles all the placeholders defined in the different contexts

For a quick look at the workflow defined by this proposal, see 
[main.cpp](main.cpp)

All the code compiles, and runs, although it deals with memory management only, and therefore does not build nor compute any stencil. 

Note: all information about the fields contained in the repositories is statically generated at compile time, from the enum classes of each `context` and the additional mapping of each storage to its storage type contained in `<context>_repo_info_t` of [param_definitions.hpp](param_definitions.hpp). This approach has several drawbacks:
 1. The field_pool as well as the repositories contain metaprogramming that might affect the compilation time of each translation unit.
 2. Tracer fields are pushed by the model at runtime. Current infrastructure should be extended in order to have identifiers in the enum classes of the `context` for container of storages instead of storages in order to support dynamic tracers. 

In order to alleviate these issues, there are two alternative designs that are sketched in the following: 

Runtime storages
=================
The container of the storages could be converted into runtime containers, accessed by name of the field (instead of the current enum class).
The repositories will store the different storage type objects in multiple unordered_map:
```
std::unordered_map<std::string, data_store_ijk_t> ijk_fields_t;
std::unordered_map<std::string, data_store_ij_t> ij_fields_t;
```

The consequence of this in the API, is that the type of the storage need to be specify when accessing storages. 
Like that 
```
  auto u = fpool.get_st<dycore_param, dycore_param::u>();
```
becomes

```
  auto u = fpool.get_st<data_store_ijk_t>("u");
```

Still the placeholders can not store in runtime containers since the type of each placeholder is different, even for fields that share the same storage type. 

Runtime binding of the placeholders
==================

These other alternative proposes to do the binding between the placeholder and the storage using a runtime API. 
In this proposal, the placeholders are not stored nor managed by the field_pool but instead they are locally instantiated in the scope of construction of the stencil.

```
auto p_data = fpool.get_arg<vadvect, vadvect::data>();
    auto p_fc = fpool.make_arg<data_store_ijk_t>("fc");
    //...
    gridtools::aggregator_type<  > domain(p_fc() = fc, /*...*/ );

    m_stencil = gridtools::make_computation<backend_t>(...);
```
Since the placeholders are destructured when they go out of scope, we need another mechanism of redoing the bindings between the placeholder and the storages. 
The following api achieves this by using a placeholder name as key to identify the placeholder:
```
   m_stencil.bind("fc", in);
```

This approach requires some development work in the GridTools backend infrastructure.

