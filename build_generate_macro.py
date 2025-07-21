def generate_macro_line(n):
    members = [f"m{i+1}" for i in range(n)]
    return f"GET_MEMBER_TUPLE_HELPER({n}, {', '.join(members)})"

def generate_header_file(filename="./tinyrefl/reflection_get_member_tuple_helper.hpp", max_count=255):
    with open(filename, "w") as f:
        f.write("// Auto-generated macro expansion for GET_MEMBER_TUPLE_HELPER\n")
        f.write("#pragma once\n\n")
        for i in range(1, max_count + 1):
            f.write(generate_macro_line(i) + "\n")

if __name__ == "__main__":
    generate_header_file()
