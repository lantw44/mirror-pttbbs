
Use Case:
  list:
    �C�X�Ҧ�������b�����x�C
  query site:
    �C�X site �o�ӯ�����T�A�]�A�ײv���C
  quit:
    ���_�s�u���}�C




   +-------+                                             +-------+ 
 +-------+ |    +----------+            +----------+   +-------+ |
 | client|-+ ---| bankd(1) | <--------> | bankd(2) |---| client|-+
 +-------+      +----------+            +----------+   +-------+

 client: 

Note:
  client ����� source�A�ҥH�аȥ��`�N server �@�w�n�p�ߨ��� client
  ���c�d

Protocol (between banks):
  1.���] bankd(1) �� Ptt �o�ݪ� daemon�Abankd(2) �� Ptt2 �o�ݪ��C
  2.�� Ptt �W���H�Q�׿��� Ptt2 �W���H�APtt user(mbbsd) �o��|�}�һP
    ���w�����W bank ���s�u�C[1]
  3.Client �������ۤv����[2]�A�A�� server ���L�n��b����H����B�C
  4.Server �i��T�{�ʧ@
    1) �T�w���������ײv�P�ۤv�ۦP�C���P�h�^�����~�C
    2) �T�w���H�O�_�s�b�C�O�A�h�N���פJ�A�^�����\�T���F�_�A�h�^����
       �~�C
  5.�ھ� server ���^�����X�ʧ@�C���\�h��������ơA���ѴN�N�ۤv����
    �[�^�ӡC


 [1] ���[�K����]�O ... �i :p ���ݭn���ܪ����� ssh tunnel �ӥΧa :p
 [2] �קK���H�e������c�N�_�u�A�y�������W�L��C
