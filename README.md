wkb_wrapper
===========

Operation on data in WKB format

Example
=======
```
char *data = ....
ml::wkb shape(data);
std::cout << ml::wkb::type_to_text(shape.type()) << std::endl;
std::cout << shape.bounds() << std::endl;
```
