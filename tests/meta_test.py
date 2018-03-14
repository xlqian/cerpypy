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
    Hey = CerpypyField(sub_field=None, object_type='array', getter=itemgetter('Hey'))
    Hello = CerpypyField(sub_field=subRoot, object_type='object', getter=itemgetter('Hello'))
    A_World = CerpypyField(sub_field=subRoot, object_type='array', getter=itemgetter('A_World'))
    B_World = CerpypyField(sub_field=subRoot, object_type='array', getter=itemgetter('B_World'))

import timeit

d = {
    'Hey': [1, 2, 3, 4, 5, 6, 7, 8],
    'Hello': {
        'Here': 1,
        'There': 2
    },
    'A_World': {
        'Here': 1,
        'There': 2
    },
    'B_World': {
        'Here': 'titi',
        'There': 'toto'
    }
}

def test():
     print cerpypy.JsonMakerCaller("Root").make(d)
test()

import serpy

class S(serpy.DictSerializer):
    Here = serpy.Field()
    There = serpy.Field()

class D(serpy.DictSerializer):
    Hey = serpy.Serializer(many=True)
    Hello = S()
    A_World = S()
    B_World = S()


def test1():
    cerpypy.JsonMakerCaller("Root").make(d)


def test2():
    D(d).data


#print(cerpypy.JsonMakerCaller("Root").make(d))
print(timeit.timeit(test1))
print(timeit.timeit(test2))
