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
        cerpypy.register_pyobj_maker(name, json_maker_args)
        return super(Meta, mcs).__new__(mcs, name, bases, dct)

from operator import itemgetter

class subRoot2(with_metaclass(Meta, object)):
    Greeting = CerpypyField(object_type='object', getter=itemgetter('Greeting'))
    Fine = CerpypyField(object_type='object', getter=itemgetter('Fine'))

class subRoot(with_metaclass(Meta, object)):
    Here = CerpypyField(object_type='object', getter=itemgetter('Here'))
    There = CerpypyField(object_type='object', getter=itemgetter('There'))


class Root(with_metaclass(Meta, object)):
    Hey = CerpypyField(sub_field=subRoot2, object_type='array', getter=itemgetter('Hey'))
    Hello = CerpypyField(sub_field=subRoot, object_type='object', getter=itemgetter('Hello'))
    A_World = CerpypyField(sub_field=subRoot, object_type='object', getter=itemgetter('A_World'))
    B_World = CerpypyField(sub_field=subRoot, object_type='array', getter=itemgetter('B_World'))

import timeit

d = {
    'Hey': [{
        'Greeting': 42,
        'Fine': 43
    }]*20,
    'Hello': {
        'Here': 1,
        'There': 2
    },
    'A_World': {
        'Here': 1,
        'There': 2
    },
    'B_World': [{
        'Here': 42,
        'There': 43
    }]*50
}



def test1():
    return cerpypy.JsonMakerCaller("Root").make(d)

def test2():
    return cerpypy.PyObjMakerCaller("Root").make(d)

import serpy

class S(serpy.DictSerializer):
    Here = serpy.Field()
    There = serpy.Field()

class S1(serpy.DictSerializer):
    Greeting = serpy.Field()
    Fine = serpy.Field()

class D(serpy.DictSerializer):
    Hey = S1(many=True)
    Hello = S()
    A_World = S()
    B_World = S(many=True)


import ujson


def test3():
    return (D(d).data)


print('test1', test1())
print('test2', test2())
print('test3', test3())

t1 = timeit.timeit(test1, number=1000000)
t2 = timeit.timeit(test2, number=1000000)
t3 = timeit.timeit(test3, number=1000000)

print "t1: {}, t3 {}, t1/t3: {}".format(t1, t3, (t1/t3))
print "t2: {}, t3 {}, t1/t3: {}".format(t2, t3, (t2/t3))
