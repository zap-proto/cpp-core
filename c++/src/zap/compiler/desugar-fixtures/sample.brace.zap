@0xbf5147cbbecf40c1;

struct Person {
  name @0 :Text;
  age @1 :UInt32;
  email @2 :Text;
  union {
    school @3 :Text;
    work @4 :Text;
  }
  hobbies @5 :List(Text);
}

enum Color {
  red @0;
  green @1;
  blue @2;
}

interface Greeter {
  hello @0 (name :Text) -> (greeting :Text);
  bye @1 () -> ();
}
