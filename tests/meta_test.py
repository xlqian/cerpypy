import cerpypy

from six import with_metaclass
from collections import namedtuple

CerpypyField = namedtuple('CerpypyField', ['name', 'getter', 'sub_field', 'object_type'])
CerpypyField.__new__.__defaults__ = (None, None, None, 'object')


class Meta(type):
    def __new__(mcs, name, bases, dct):
        json_maker_args = []
        for k, v in dct.items():
            if type(v) == CerpypyField:
                if v.sub_field is None:
                    maker_name = "{}:{}".format(name, k)
                else:
                    assert type(v.sub_field) == Meta
                    maker_name = v.sub_field.__name__

                assert maker_name
                json_maker_args.append((k, maker_name, v.getter, v.object_type))

        cerpypy.register_json_maker(name, json_maker_args)
        return super(Meta, mcs).__new__(mcs, name, bases, dct)

from operator import itemgetter


class subRoot(with_metaclass(Meta, object)):
    Here = CerpypyField(object_type='object', getter=itemgetter('Here'))
    There = CerpypyField(object_type='array', getter=itemgetter('There'))


class Root(with_metaclass(Meta, object)):
    Hey = CerpypyField(sub_field=None, object_type='object', getter=itemgetter('Hey'))
    Hello = CerpypyField(sub_field=subRoot, object_type='object', getter=itemgetter('Hello'))
    World = CerpypyField(sub_field=subRoot, object_type='array', getter=itemgetter('World'))

import timeit

d = {
    'Hey': 'hey',
    'Hello': {
        'Here': 1,
        'There': 2
    },
    'World': {
        'Here': 1,
        'There': 2
    }
}

def test1():
     cerpypy.JsonMakerCaller("Root").make(d)
test1()
import serpy

class S(serpy.DictSerializer):
    Here = serpy.Field()
    There = serpy.Field()

class D(serpy.DictSerializer):
    Hey = serpy.Field()
    Hello = S()
    World = S()

def test2():
    D(d).data


#print(cerpypy.JsonMakerCaller("Root").make(d))
print(timeit.timeit(test1))
print(timeit.timeit(test2))
