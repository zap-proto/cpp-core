@0xbf5147cbbecf40c1;

struct Person
  name Text
  age UInt32
  email Text
  union
    school Text
    work Text
  hobbies List(Text)

enum Color
  red
  green
  blue

interface Greeter
  hello (name :Text) -> (greeting :Text)
  bye () -> ()
