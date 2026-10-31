/* The probe intercepts only BDOS 12, 24 and 108 during shutdown. */
int main(int argc, char **argv)
{
    return argc > 1 ? -1 : 37;
}
